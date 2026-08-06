# C++ Refactoring Checklist & Pattern Reference

This reference provides actionable C++20 code transformation patterns, anti-pattern detection rules, and Before vs. After code examples for high-engineering C++ refactoring, method decomposition, human-centric naming, and expressive Ruby/Crystal-style readability.

---

## 1. Method Decomposition & Single Level of Abstraction (SLAP)

### Anti-Pattern: Monolithic Function (Mixed Abstraction Levels & High Cognitive Complexity)
```cpp
// ❌ BAD: Mixed high-level logic, low-level math, and UI/SFX side effects
void Player::applyDamage(int raw_damage, DamageType type, bool is_critical) {
    if (m_is_dead || m_is_invulnerable) return;

    float final_dmg = static_cast<float>(raw_damage);
    if (type == DamageType::Physical) {
        float armor_reduction = static_cast<float>(m_armor) * 0.75f;
        final_dmg = std::max(0.0f, final_dmg - armor_reduction);
    } else if (type == DamageType::Elemental) {
        final_dmg *= (1.0f - m_elemental_res);
    }

    if (is_critical) final_dmg *= 1.5f;

    int int_dmg = static_cast<int>(final_dmg);
    m_hp = std::max(0, m_hp - int_dmg);

    if (m_hp == 0) {
        m_is_dead = true;
        m_anim_state = AnimState::Death;
        m_audio_engine.playSFX("player_death.wav");
    } else {
        m_anim_state = AnimState::Hurt;
        m_audio_engine.playSFX("player_hurt.wav");
    }
}
```

### Clean Pattern: Composed Method Pattern + Anonymous Namespace Helpers
```cpp
//  GOOD: Pure algorithmic helpers encapsulated in anonymous namespace in .cpp
namespace {

[[nodiscard]] constexpr float calculateTypeMitigatedDamage(float base_dmg, DamageType type, int armor, float elemental_res) noexcept {
    if (type == DamageType::Physical) {
        const float armor_reduction = static_cast<float>(armor) * 0.75f;
        return std::max(0.0f, base_dmg - armor_reduction);
    }
    if (type == DamageType::Elemental) {
        return base_dmg * (1.0f - elemental_res);
    }
    return base_dmg;
}

[[nodiscard]] constexpr int computeFinalDamageAmount(int raw_damage, DamageType type, int armor, float elemental_res, bool is_critical) noexcept {
    const float mitigated = calculateTypeMitigatedDamage(static_cast<float>(raw_damage), type, armor, elemental_res);
    const float critical_multiplier = is_critical ? 1.5f : 1.0f;
    return static_cast<int>(mitigated * critical_multiplier);
}

} // anonymous namespace

// Composed Method operating at uniform high abstraction level
void Player::applyDamage(int raw_damage, DamageType type, bool is_critical) {
    if (cannotTakeDamage()) return;

    const int damage_amount = computeFinalDamageAmount(raw_damage, type, m_armor, m_elemental_res, is_critical);
    deductHealth(damage_amount);
    updateHurtOrDeathState();
}

bool Player::cannotTakeDamage() const noexcept {
    return m_is_dead || m_is_invulnerable;
}

void Player::deductHealth(int amount) noexcept {
    m_hp = std::max(0, m_hp - amount);
}

void Player::updateHurtOrDeathState() {
    if (m_hp == 0) {
        m_is_dead = true;
        m_anim_state = AnimState::Death;
        m_audio_engine.playSFX("player_death.wav");
        return;
    }
    m_anim_state = AnimState::Hurt;
    m_audio_engine.playSFX("player_hurt.wav");
}
```

---

## 2. Guard Clauses & Bouncer Pattern (Flattening Nesting)

### Anti-Pattern: Deep Arrow Anti-Pattern
```cpp
// ❌ BAD: Deeply nested conditionals with logic buried at level 4
void Inventory::equipItem(Item* item, int slot_idx) {
    if (item != nullptr) {
        if (slot_idx >= 0 && slot_idx < MAX_SLOTS) {
            if (item->isEquippable()) {
                if (m_player_level >= item->getRequiredLevel()) {
                    m_slots[slot_idx] = item;
                    recalculateStats();
                } else {
                    logError("Level too low");
                }
            } else {
                logError("Item not equippable");
            }
        } else {
            logError("Invalid slot index");
        }
    }
}
```

### Clean Pattern: Guard Clauses with Early Returns
```cpp
//  GOOD: Preconditions handled upfront; happy path executed at zero indentation
void Inventory::equipItem(Item* item, int slot_idx) {
    if (item == nullptr) return;
    if (slot_idx < 0 || slot_idx >= MAX_SLOTS) {
        logError("Invalid slot index");
        return;
    }
    if (!item->isEquippable()) {
        logError("Item not equippable");
        return;
    }
    if (m_player_level < item->getRequiredLevel()) {
        logError("Level too low");
        return;
    }

    m_slots[slot_idx] = item;
    recalculateStats();
}
```

---

## 3. Human-Centric Naming & Purging AI Verbosity

### Anti-Pattern: AI Noise Words & Scope Redundancy
```cpp
// ❌ BAD: AI-generated noisy boilerplate with redundant context
class PlayerInventoryManagerService {
public:
    std::vector<ItemObjectData> m_playerInventoryItemObjectDataVector;
    int m_maxCapacityOfInventoryItemsLimitInteger;

    void addItemObjectToPlayerInventoryManager(const ItemObjectData& itemObjectDataData);
    bool checkIfPlayerInventoryManagerIsFullCheck();
};
```

### Clean Pattern: Human-Centric Concise Naming
```cpp
//  GOOD: Context provided by class scope; concise domain terms
class Inventory {
public:
    explicit Inventory(std::size_t capacity) : m_capacity(capacity) {}

    [[nodiscard]] bool add(Item item);
    [[nodiscard]] bool is_full() const noexcept { return m_items.size() >= m_capacity; }
    [[nodiscard]] std::span<const Item> items() const noexcept { return m_items; }

private:
    std::size_t m_capacity{0};
    std::vector<Item> m_items;
};
```

---

## 4. Ruby/Crystal-Style Expressive Readability in C++20

### Pattern 4A: Predicate Queries & C++20 Ranges Pipelines
```cpp
// ❌ BAD: Imperative loop with raw flag checks
std::vector<std::string> get_active_admin_names(const std::vector<User>& users) {
    std::vector<std::string> result;
    for (size_t i = 0; i < users.size(); ++i) {
        if (users[i].get_status() == 1 && users[i].get_role_id() == 99 && !users[i].get_is_banned_flag()) {
            result.push_back(users[i].get_user_name());
        }
    }
    return result;
}

//  GOOD: Ruby-like predicate methods & C++20 Ranges pipeline
class User {
public:
    [[nodiscard]] bool is_active() const { return m_status == Status::Active; }
    [[nodiscard]] bool is_admin() const { return m_role == Role::Admin; }
    [[nodiscard]] bool is_banned() const { return m_is_banned; }
    [[nodiscard]] bool can_access_dashboard() const { return is_active() && is_admin() && !is_banned(); }
    [[nodiscard]] std::string_view name() const { return m_name; }
private:
    enum class Status { Inactive, Active };
    enum class Role { Member, Admin };
    Status m_status{Status::Active};
    Role m_role{Role::Admin};
    bool m_is_banned{false};
    std::string m_name;
};

// Declarative pipeline: users | filter(&User::can_access_dashboard) | transform(&User::name)
auto get_active_admin_names(const std::vector<User>& users) {
    return users 
        | std::views::filter(&User::can_access_dashboard)
        | std::views::transform(&User::name);
}
```

### Pattern 4B: C++20 Designated Initializers (Eliminating Argument Traps)
```cpp
// ❌ BAD: Cryptic positional arguments with boolean parameter trap
void configure_client(std::string host, int port, bool use_ssl, bool verify_peer, int timeout_sec);
configure_client("api.aetherlux.io", 8080, true, false, 30); // What do true, false mean?

//  GOOD: C++20 Designated Initializers (Self-Documenting Named Arguments)
struct ClientConfig {
    std::string host{"localhost"};
    int port{8080};
    bool use_ssl{true};
    bool verify_peer{true};
    std::chrono::seconds timeout{30};
};

void configure_client(ClientConfig config);

// Call site reads naturally:
configure_client({
    .host = "api.aetherlux.io",
    .port = 8080,
    .use_ssl = true,
    .verify_peer = false,
    .timeout = std::chrono::seconds{30}
});
```

### Pattern 4C: Strongly Typed Quantities & User-Defined Literals (UDLs)
```cpp
// ❌ BAD: Naked floats with magic calculations
void heal(Player& player, float amount, float duration_secs) {
    player.hp += amount * 0.75f;
}
heal(player, 100.0f, 10.0f);

//  GOOD: Strongly-typed quantities and Ruby-style UDLs
struct HealthPoints { float value{0.0f}; };
struct Efficiency { float ratio{1.0f}; };

constexpr HealthPoints operator""_hp(unsigned long long val) { return HealthPoints{static_cast<float>(val)}; }
constexpr Efficiency operator""_percent(unsigned long long val) { return Efficiency{static_cast<float>(val) / 100.0f}; }

void heal(Player& player, HealthPoints base, Efficiency efficiency, std::chrono::seconds duration) {
    player.hp += base.value * efficiency.ratio;
}

// Call site:
heal(player, 100_hp, 75_percent, std::chrono::seconds{10});
```

---

## 5. Law of Demeter (Train Wreck Call Elimination)

### Anti-Pattern: Reaching Through Object Graphs
```cpp
// ❌ BAD: Reaching deep into internal sub-objects (Train Wreck)
void Player::renderTile(World& world, int x, int y) {
    world.getMap().getTileGrid().getTile(x, y).getRenderer().drawTilePixel(x, y);
}
```

### Clean Pattern: Delegation & Direct Parameter Passing
```cpp
//  GOOD: Delegate action directly to immediate collaborator
void Room::drawTile(int x, int y, Renderer& renderer) {
    const Tile& tile = m_grid.at(x, y);
    renderer.draw(tile);
}
```

---

## 6. Zero Magic Numbers & Dynamic Layout Math

```cpp
// ❌ BAD: Raw hardcoded magic literals
float x1 = px + 16 - 4;
float y1 = py + 32;

//  GOOD: Named constexpr constants & dynamic container bounds
constexpr float kCharacterHalfWidth = 8.0f;
const float center_x = player_bounds.x + (player_bounds.width * 0.5f) - kCharacterHalfWidth;
const float feet_y   = player_bounds.y + player_bounds.height;
```

---

## 7. Software Rendering Clipping Checklist

When writing to raw 1D pixel buffers (`uint32_t* pixels` representing a 2D `width x height` grid):
```cpp
// Always enforce clip rectangle checks before writing
if (pixel_x >= 0 && pixel_x < screen_width && pixel_y >= 0 && pixel_y < screen_height) {
    pixels[pixel_y * screen_width + pixel_x] = color;
}
```
