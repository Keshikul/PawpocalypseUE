extends SceneTree

# Ground truth for PawCore's PcCombat: Godot's own CombatResolver (scripts/autoload/combat_resolver.gd in a scratch copy of the project)
# with the shipping combat_constants.tres, over random CombatStats snapshots spanning the game's magnitudes. Every value
# is written as the exact encoded Variant (hex), so the C++ test compares bit for bit.

const FIELDS := ["paw_damage_multiplier", "attack_speed_multiplier", "shred_per_stack", "execute_threshold",
	"aura_damage_multiplier", "aura_radius_branch_multiplier", "evolution_damage_multiplier", "weapon_multiplier",
	"additive_damage_sum", "multiplicative_damage_product", "crit_chance", "crit_multiplier", "overkill_fraction",
	"kitten_power_multiplier", "kitten_additive_damage_sum", "kitten_multiplicative_damage_product", "poop_power_multiplier",
	"poop_additive_damage_sum", "poop_multiplicative_damage_product", "kitten_speed_multiplier", "shield_pierce_fraction",
	"pounce_power_multiplier", "pounce_cooldown_reduction", "reawaken_multiplier"]


func h(x) -> String:
	return var_to_bytes(float(x)).hex_encode()


func _init() -> void:
	var Resolver := load("res://scripts/autoload/combat_resolver.gd")
	var StatsScript := load("res://scripts/systems/combat_stats.gd")
	var r = Resolver.new()
	r.constants = load("res://data/combat_constants.tres")
	var shipped_exponent: float = r.constants.overkill_bounty_exponent
	var rng := RandomNumberGenerator.new()
	rng.seed = 20261028
	var out := FileAccess.open("res://combat_fixture.tsv", FileAccess.WRITE)
	out.store_line("# " + "\t".join(PackedStringArray(["aura_owned", "flat_bounty"] + FIELDS + ["stacks", "vulnerable", "rate", "excess", "max_hp", "reward",
		"damage_scale", "strike", "strikes_per_sec", "paw_dps", "aura_dps", "player_dps", "kitten_dps", "pounce", "pounce_multiple",
		"pounce_cap_scale", "pounce_cooldown", "overkill_damage", "overkill_bounty", "poop_dps"])))
	for i in range(5000):
		var s = StatsScript.new()
		s.paw_damage_multiplier = pow(10.0, rng.randf_range(0.0, 9.0))
		s.attack_speed_multiplier = rng.randf_range(0.5, 6.0)
		s.shred_per_stack = rng.randf_range(0.0, 0.2) if rng.randf() < 0.7 else 0.0
		s.execute_threshold = rng.randf_range(0.0, 0.3)
		s.aura_owned = rng.randf() < 0.6
		s.aura_damage_multiplier = rng.randf_range(1.0, 50.0)
		s.aura_radius_branch_multiplier = rng.randf_range(1.0, 3.0)
		s.evolution_damage_multiplier = pow(r.constants.evolution_damage_per_stage, rng.randi_range(0, 6))
		s.weapon_multiplier = rng.randf_range(1.0, 1.4)
		s.additive_damage_sum = rng.randf_range(0.0, 5.0)
		s.multiplicative_damage_product = rng.randf_range(1.0, 3.0)
		s.crit_chance = rng.randf_range(0.0, 1.0)
		s.crit_multiplier = rng.randf_range(1.5, 4.0)
		s.overkill_fraction = rng.randf_range(0.0, 0.8) if rng.randf() < 0.8 else 0.0
		s.kitten_power_multiplier = pow(10.0, rng.randf_range(0.0, 8.0))
		s.kitten_additive_damage_sum = rng.randf_range(0.0, 3.0)
		s.kitten_multiplicative_damage_product = rng.randf_range(1.0, 2.0)
		s.poop_power_multiplier = rng.randf_range(1.0, 2.0)
		s.poop_additive_damage_sum = rng.randf_range(0.0, 2.0)
		s.poop_multiplicative_damage_product = rng.randf_range(1.0, 5.0)
		s.kitten_speed_multiplier = rng.randf_range(1.0, 2.0)
		s.shield_pierce_fraction = rng.randf_range(0.0, 0.5)
		s.pounce_power_multiplier = rng.randf_range(1.0, 6.0)
		# Includes reductions at and past 1.0, where the cooldown reaches zero and the multiple goes infinite.
		s.pounce_cooldown_reduction = rng.randf_range(0.0, 1.05)
		s.reawaken_multiplier = pow(10.0, rng.randf_range(0.0, 3.0))
		var flat := rng.randf() < 0.5
		r.constants.overkill_bounty_exponent = 0.0 if flat else shipped_exponent
		var stacks := rng.randi_range(-2, 12)
		var vulnerable := rng.randf() < 0.3
		var rate := 1.0 if rng.randf() < 0.5 else rng.randf_range(1.0, 3.0)
		var excess := pow(10.0, rng.randf_range(-2.0, 15.0)) if rng.randf() < 0.9 else -1.0
		var max_hp := pow(10.0, rng.randf_range(0.0, 13.0))
		var reward := pow(10.0, rng.randf_range(-1.0, 12.0))
		var cols := PackedStringArray(["1" if s.aura_owned else "0", "1" if flat else "0"])
		for f in FIELDS:
			cols.append(h(s.get(f)))
		cols.append(str(stacks))
		cols.append("1" if vulnerable else "0")
		cols.append(h(rate))
		cols.append(h(excess))
		cols.append(h(max_hp))
		cols.append(h(reward))
		cols.append(h(r._damage_scale(s)))
		cols.append(h(r.paw_strike_damage(s, stacks)))
		cols.append(h(r.strikes_per_sec(s)))
		cols.append(h(r.paw_dps(s)))
		cols.append(h(r.aura_dps(s)))
		cols.append(h(r.player_dps(s)))
		cols.append(h(r.kitten_dps(s)))
		cols.append(h(r.pounce_damage(s, vulnerable, rate)))
		cols.append(h(r.pounce_aura_multiple(s, rate)))
		cols.append(h(r.pounce_cap_scale(s, rate)))
		cols.append(h(r.pounce_cooldown(s)))
		cols.append(h(r.overkill_damage(s, excess)))
		cols.append(h(r.overkill_bounty(s, excess, max_hp, reward)))
		cols.append(h(r.poop_dps(s)))
		out.store_line("\t".join(cols))
	out.close()
	r.free()
	print("COMBAT_FIXTURE wrote 5000 rows")
	quit(0)
