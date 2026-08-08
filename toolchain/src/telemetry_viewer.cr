require "json"

module Alx
  class TelemetryViewer
    TELEMETRY_PATH = "/tmp/alx_telemetry.json"

    struct TelemetryData
      include JSON::Serializable
      property ticks : Int64?
      property sim_time_sec : Float64?
      property time_scale : Float64?
      property paused : Bool?
      property twilight_level : Float64?
      property twilight_pct : Int32?
      property twilight_delta_per_sec : Float64?
      property twilight_rolling_rate_per_sec : Float64?
      property twilight_rolling_rate_15s_per_sec : Float64?
      property twilight_session_net_rate_per_sec : Float64?
      property last_twilight_event_delta : Float64?
      property last_twilight_event_cause : String?
      property seconds_since_last_event : Float64?
      property dark_towers_count : Int32?
      property shadow_eggs_count : Int32?
      property enemies_count : Int32?
      property spires_count : Int32?
      property refiners_count : Int32?
      property pipes_count : Int32?
      property spires_cleanse_rate_per_sec : Float64?
      property player_hp : Int32?
      property player_max_hp : Int32?
      property player_alloy : Int32?
    end

    def self.run
      Signal::INT.trap { exit }
      Signal::TERM.trap { exit }

      puts "\e[?25l" # Hide cursor
      at_exit { puts "\e[?25h\e[0m" } # Restore cursor on exit

      loop do
        render
        sleep 0.1.seconds
      rescue ex
        # Keep loop alive if file is temporarily locked or incomplete
        sleep 0.1.seconds
      end
    end

    def self.render
      unless File.exists?(TELEMETRY_PATH)
        print "\e[H\e[2J"
        puts "\e[1;33mAetherlux Telemetry Viewer\e[0m"
        puts "Waiting for telemetry output at #{TELEMETRY_PATH}..."
        puts "Launch the game or run a headless simulation to see live data."
        return
      end

      content = File.read(TELEMETRY_PATH)
      data = TelemetryData.from_json(content)

      sim_time = data.sim_time_sec || 0.0
      mins = (sim_time / 60).to_i
      secs = (sim_time % 60).to_i

      twilight_val = data.twilight_level || 0.0
      twilight_pct = data.twilight_pct || (twilight_val * 100).to_i

      bar_width = 30
      filled_len = (twilight_val * bar_width).round.to_i.clamp(0, bar_width)
      empty_len = bar_width - filled_len
      bar_str = "=" * filled_len + "." * empty_len

      # Color coding for twilight
      tw_color = if twilight_pct >= 75
                   "\e[1;31m" # Red
                 elsif twilight_pct >= 40
                   "\e[1;33m" # Yellow
                 else
                   "\e[1;32m" # Green
                 end

      print "\e[H\e[2J"
      puts "\e[1;36m=======================================================================\e[0m"
      puts "\e[1;36m                       AETHERLUX LIVE TELEMETRY                        \e[0m"
      puts "\e[1;36m=======================================================================\e[0m"
      printf " Sim Time:   %02d:%02d        | Ticks:      %-10d | Speed Scale: %.1fx\n",
        mins, secs, data.ticks || 0, data.time_scale || 1.0
      printf " State:      %-10s | Player HP:  %d/%d        | Cursed Alloy: %d\n",
        (data.paused ? "\e[33mPAUSED\e[0m" : "\e[32mRUNNING\e[0m"),
        data.player_hp || 0, data.player_max_hp || 0, data.player_alloy || 0
      puts "\e[1;36m-----------------------------------------------------------------------\e[0m"
      puts " TWILIGHT LEVEL & NET FLOW DYNAMICS:"
      printf " Current Level:     [%s] %s%3d%%\e[0m (Raw: %.4f)\n", bar_str, tw_color, twilight_pct, twilight_val

      rolling_dt = data.twilight_rolling_rate_per_sec || 0.0
      rolling_color = rolling_dt > 0.0001 ? "\e[1;31m" : (rolling_dt < -0.0001 ? "\e[1;32m" : "\e[37m")
      rolling_status = rolling_dt > 0.0001 ? "Corrupting" : (rolling_dt < -0.0001 ? "Cleansing" : "Stable")
      printf " Rolling Rate (3s):  %s%+.4f/sec (%s)\e[0m\n", rolling_color, rolling_dt, rolling_status

      rolling_15s_dt = data.twilight_rolling_rate_15s_per_sec || 0.0
      rolling_15s_color = rolling_15s_dt > 0.0001 ? "\e[1;31m" : (rolling_15s_dt < -0.0001 ? "\e[1;32m" : "\e[37m")
      rolling_15s_status = rolling_15s_dt > 0.0001 ? "Corrupting" : (rolling_15s_dt < -0.0001 ? "Cleansing" : "Stable")
      printf " Rolling Rate (15s): %s%+.4f/sec (%s)\e[0m\n", rolling_15s_color, rolling_15s_dt, rolling_15s_status

      session_dt = data.twilight_session_net_rate_per_sec || 0.0
      session_color = session_dt > 0.0001 ? "\e[1;31m" : (session_dt < -0.0001 ? "\e[1;32m" : "\e[37m")
      session_status = session_dt > 0.0001 ? "Net Corrupting" : (session_dt < -0.0001 ? "Net Purifying" : "Net Neutral")
      printf " Session Net Rate:   %s%+.4f/sec (%s)\e[0m\n", session_color, session_dt, session_status

      last_delta = data.last_twilight_event_delta || 0.0
      last_cause = data.last_twilight_event_cause || "None"
      last_ago = data.seconds_since_last_event || 0.0
      printf " Last Event:         %+.4f [%s] (%.1fs ago)\n", last_delta, last_cause, last_ago
      puts "\e[1;36m-----------------------------------------------------------------------\e[0m"
      puts " SHADOW & THREAT SYSTEM:"
      printf " Dark Towers:  %-3d  | Shadow Eggs: %-3d  | Active Enemies: %-3d\n",
        data.dark_towers_count || 0, data.shadow_eggs_count || 0, data.enemies_count || 0
      puts "\e[1;36m-----------------------------------------------------------------------\e[0m"
      puts " MANA INFRASTRUCTURE:"
      printf " Light Spires: %-3d  | Refiners:    %-3d  | Pipes:          %-3d\n",
        data.spires_count || 0, data.refiners_count || 0, data.pipes_count || 0
      printf " Total Cleanse Rate: %.4f/sec\n", data.spires_cleanse_rate_per_sec || 0.0
      puts "\e[1;36m=======================================================================\e[0m"
      puts " Press Ctrl+C to exit viewer."
    end
  end
end

Alx::TelemetryViewer.run
