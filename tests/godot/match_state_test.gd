## The crystal core match's rules (server/match_state.gd), without a network:
##   godot --headless --path server --script <absolute path of this file>
## Prints each check and "MATCH TESTS: n failed"; the exit code is the number failed.
extends SceneTree

const MatchState := preload("res://match_state.gd")

var _failed := 0
var _checked := 0


func _initialize() -> void:
	_host_and_start()
	_placement()
	_damage_rules()
	_protection()
	_out_and_winner()
	_away_and_back()
	_back_to_lobby()
	_free_build()
	_veins()
	_buildings()
	_blueprints()
	_shrines()
	_units()
	_wilds()
	print("MATCH TESTS: %d of %d failed" % [_failed, _checked])
	quit(_failed)


func _check(what: String, good: bool) -> void:
	_checked += 1
	if not good:
		_failed += 1
	print("%s  %s" % ["ok  " if good else "FAIL", what])


## Three players in a lobby: a (peer 11) came first and is the host.
func _three() -> RefCounted:
	var state: RefCounted = MatchState.new()
	state.free_build = false  # the rounds (free building has its own test below)
	state.use_veins = false   # (the veins too)
	state.join("a", 11, "Anna", 0)
	state.join("b", 12, "Bert", 1)
	state.join("c", 13, "Cleo", 2)
	return state


## Three in a fight, cores 100 m apart, protection over, every hero beside the core he strikes.
func _fighting() -> RefCounted:
	var state := _three()
	state.rules["protection_seconds"] = 0.0
	state.start(11, 0.0)
	state.place(11, Vector3(0, 0, 0))
	state.place(12, Vector3(100, 0, 0))
	state.place(13, Vector3(0, 0, 100))
	state.tick(90.0)
	return state


func _host_and_start() -> void:
	var state := _three()
	_check("the first who came is the host", state.host() == "a")
	_check("only the host starts", state.start(12, 0.0) != "")
	_check("the host starts", state.start(11, 0.0) == "" and state.phase == MatchState.PLACEMENT)
	_check("not twice", state.start(11, 1.0) != "")
	var alone: RefCounted = MatchState.new()
	alone.free_build = false
	alone.use_veins = false
	alone.join("a", 11, "Anna", 0)
	_check("one player is not enough", alone.start(11, 0.0) != "")
	state.leave(11, 2.0)
	_check("the host passes on when he leaves in the lobby", _three_left_host())
	state.join("late", 14, "Late", 0)
	_check("who comes during a match watches", not state.players["late"]["playing"])


func _three_left_host() -> bool:
	var state := _three()
	state.leave(11, 1.0)
	return state.host() == "b"


func _placement() -> void:
	var state := _three()
	state.start(11, 0.0)
	_check("a core is placed", state.place(11, Vector3(0, 0, 0)) == "")
	_check("only one each", state.place(11, Vector3(200, 0, 0)) != "")
	_check("not within the spacing", state.place(12, Vector3(50, 0, 0)) == "Too close to another core")
	_check("80 m is enough", state.place(12, Vector3(80, 0, 0)) == "")
	_check("not off the island", state.place(13, Vector3(1200, 0, 0)) != "")
	_check("a watcher places nothing", state.place(99, Vector3(0, 0, 300)) != "")
	state.tick(89.0)
	_check("placement lasts its time", state.phase == MatchState.PLACEMENT)
	state.tick(90.0)
	_check("then the fight", state.phase == MatchState.FIGHT)
	_check("who placed none is out", state.players["c"]["out"] and not state.players["a"]["out"])
	_check("no core is placed in the fight", state.place(13, Vector3(0, 0, 300)) != "")


func _damage_rules() -> void:
	var state := _fighting()
	state.position(11, Vector3(95, 0, 0))
	state.position(12, Vector3(0, 0, 0))
	_check("a hero beside an enemy core hurts it", state.core_damage(11, "b", 40.0, 100.0) == 40.0)
	_check("its health falls", is_equal_approx(state.players["b"]["health"], 2960.0))
	_check("nobody hurts his own core", state.core_damage(12, "b", 40.0, 100.0) == 0.0)
	state.position(13, Vector3(0, 0, 0))
	_check("not from too far off", state.core_damage(13, "b", 40.0, 100.0) == 0.0)
	_check("a blow is capped", state.core_damage(11, "b", 9999.0, 101.0) == 250.0)
	_check("a second's worth is capped", state.core_damage(11, "b", 200.0, 101.2) == 150.0)
	_check("and nothing more that second", state.core_damage(11, "b", 10.0, 101.5) == 0.0)
	_check("the next second again", state.core_damage(11, "b", 10.0, 102.1) == 10.0)
	state.join("late", 14, "Late", 0)
	state.position(14, Vector3(100, 0, 5))
	_check("a watcher hurts no core", state.core_damage(14, "b", 10.0, 103.0) == 0.0)
	_check("pvp in the fight", state.fighting("a") and state.fighting("b") and not state.fighting("late"))


func _protection() -> void:
	var state := _three()
	state.start(11, 0.0)
	state.place(11, Vector3(0, 0, 0))
	state.place(12, Vector3(100, 0, 0))
	state.tick(90.0)
	state.position(11, Vector3(95, 0, 0))
	state.position(12, Vector3(5, 0, 0))
	_check("a protected core takes nothing", state.core_damage(11, "b", 40.0, 100.0) == 0.0)
	_check("protection lasts five minutes", state.snapshot(100.0)["players"][0]["protected_for"] > 279.0)
	state.players["b"]["protected_until"] = 0.0
	_check("a blow on another's core ...", state.core_damage(11, "b", 30.0, 102.0) == 30.0)
	_check("... ends one's own protection", state.players["a"]["protected_until"] <= 102.0)
	_check("and the other may strike back", state.core_damage(12, "a", 30.0, 103.0) == 30.0)


func _out_and_winner() -> void:
	var state := _fighting()
	state.take_events()
	state.position(11, Vector3(95, 0, 0))
	var now := 100.0
	while state.players["b"]["core"] != null and now < 200.0:
		state.core_damage(11, "b", 250.0, now)
		now += 1.0
	_check("a destroyed core puts its owner out", state.players["b"]["out"])
	_check("the match goes on with two", state.phase == MatchState.FIGHT)
	state.position(13, Vector3(5, 0, 0))
	while state.players["a"]["core"] != null and now < 400.0:
		state.core_damage(13, "a", 250.0, now)
		now += 1.0
	_check("the last core standing wins", state.winner == "c" and state.phase == MatchState.RESULTS)
	var words := []
	for event: Dictionary in state.take_events():
		words.append(event["what"])
	_check("the events say so", words.count("out") == 2 and words.has("winner"))


func _away_and_back() -> void:
	var state := _fighting()
	state.leave(12, 100.0)
	state.tick(150.0)
	_check("away for a while is not out", not state.players["b"]["out"])
	state.join("b", 22, "Bert", 1)
	_check("he comes back on a new connection", state.key_of(22) == "b")
	state.tick(400.0)
	_check("and is still in", not state.players["b"]["out"])
	state.leave(22, 400.0)
	state.tick(521.0)
	_check("away too long is out", state.players["b"]["out"])


func _back_to_lobby() -> void:
	var state := _three()
	state.start(11, 0.0)
	state.place(11, Vector3(0, 0, 0))
	state.tick(90.0)
	_check("one core alone wins at once", state.winner == "a" and state.phase == MatchState.RESULTS)
	state.tick(109.0)
	_check("the results stay a while", state.phase == MatchState.RESULTS)
	state.tick(110.0)
	_check("then the lobby", state.phase == MatchState.LOBBY and not state.players["a"]["playing"])
	_check("and a new match can start", state.start(11, 111.0) == "")


func _free_build() -> void:
	var state: RefCounted = MatchState.new()
	state.use_veins = false
	state.join("a", 11, "Anna", 0)
	state.join("b", 12, "Bert", 1)
	_check("building freely there are no rounds", state.phase == MatchState.FIGHT and state.start(11, 0.0) != "")
	_check("a core is placed at any time", state.place(11, Vector3(0, 0, 0), 5.0) == "")
	_check("only one at a time", state.place(11, Vector3(200, 0, 0), 6.0) != "")
	_check("still not too close", state.place(12, Vector3(40, 0, 0), 6.0) != "")
	_check("protection starts when it is placed", state.players["a"]["protected_until"] == 5.0 + 300.0)
	state.position(12, Vector3(5, 0, 0))
	state.players["a"]["protected_until"] = 0.0
	_check("without a core of his own nobody strikes one", state.core_damage(12, "a", 50.0, 10.0) == 0.0)
	state.place(12, Vector3(100, 0, 0), 10.0)
	_check("with one he does", state.core_damage(12, "a", 50.0, 11.0) == 50.0)
	_check("both fight", state.fighting("a") and state.fighting("b"))
	var now := 12.0
	while state.players["a"]["core"] != null and now < 100.0:
		state.core_damage(12, "a", 250.0, now)
		now += 1.0
	_check("a destroyed core is gone, its owner not out", state.players["a"]["core"] == null and not state.players["a"]["out"])
	_check("nobody wins", state.winner == "" and state.phase == MatchState.FIGHT)
	_check("and it may be built again", state.place(11, Vector3(-150, 0, 0), now) == "")
	state.leave(12, now)
	_check("who leaves takes his core with him", not state.players.has("b") and state.healths()[0].size() == 1)


func _veins() -> void:
	var state: RefCounted = MatchState.new()
	state.join("a", 11, "Anna", 0)
	state.join("b", 12, "Bert", 1)
	var first: Dictionary = MatchState.VEINS[0]
	var second: Dictionary = MatchState.VEINS[1]
	# (Eldergrove has twice as many veins as players can be; the Sundered Isle a vein for each, four a
	# team's half, until its teams build one stronghold each.)
	_check("there are veins enough for every player", MatchState.VEINS.size() >= int(state.rules["max_players"])
			* (2 if MatchState.ISLAND == "eldergrove" else 1))
	_check("both islands keep their tables", MatchState.VEINS_ELDERGROVE.size() == 12 and MatchState.VEINS_SUNDERED.size() == 8
			and MatchState.SHRINE_SITES_SUNDERED.size() == 10)
	var apart := 1e9
	for i in MatchState.VEINS.size():
		for j in range(i + 1, MatchState.VEINS.size()):
			var u: Dictionary = MatchState.VEINS[i]
			var v: Dictionary = MatchState.VEINS[j]
			apart = minf(apart, Vector2(u["x"] - v["x"], u["z"] - v["z"]).length())
	_check("the veins keep the cores' spacing", apart >= float(state.rules["spacing"]))
	_check("no core away from a vein", state.place(11, Vector3(first["x"] + 40.0, 0, first["z"]), 1.0) != "")
	_check("a core near a vein is set on it", state.place(11, Vector3(first["x"] + 6.0, 3.0, first["z"]), 1.0) == ""
			and state.players["a"]["core"] == Vector3(first["x"], 3.0, first["z"]))
	_check("a taken vein is refused", state.place(12, Vector3(first["x"], 0, first["z"] + 4.0), 2.0) == "This vein is taken")
	_check("another vein is free", state.place(12, Vector3(second["x"], 0, second["z"]), 2.0) == "")
	_check("who owns which", state.vein_taken(0) == "a" and state.vein_taken(1) == "b" and state.vein_taken(2) == "")


func _buildings() -> void:
	var state: RefCounted = MatchState.new()
	state.use_veins = false
	state.use_blueprints = false
	state.join("a", 11, "Anna", 0)
	state.join("b", 12, "Bert", 1)
	_check("nothing is built without a core", state.build(11, "wall", Vector3(10, 0, 0), 0.0) != "")
	state.place(11, Vector3(0, 0, 0), 0.0)
	state.place(12, Vector3(200, 0, 0), 0.0)
	_check("a wall in the ring", state.build(11, "wall", Vector3(10, 0, 0), 0.5) == "")
	_check("not outside the ring", state.build(11, "wall", Vector3(40, 0, 0), 0.0) != "")
	_check("not on the core", state.build(11, "wall", Vector3(1, 0, 1), 0.0) != "")
	_check("not on another building", state.build(11, "watchtower", Vector3(10.5, 0, 0), 0.0) != "")
	_check("walls may stand end to end", state.build(11, "wall", Vector3(10, 0, 6), 0.5) == "")
	_check("a tower is set onto an anchor stone", state.build(11, "tower", Vector3(7, 0, 28), 0.0) == "")
	var tower: Dictionary = state.buildings.values().filter(func(b: Dictionary) -> bool: return b["kind"] == "tower")[0]
	_check("... exactly onto it", Vector2(tower["x"], tower["z"]).distance_to(state.anchor_spots(Vector3.ZERO)[0]) < 0.01)
	_check("one watchtower", state.build(11, "watchtower", Vector3(-15, 0, 0), 0.0) == ""
			and state.build(11, "watchtower", Vector3(0, 0, -15), 0.0) != "")
	_check("a drill beside the core", state.build(11, "drill", Vector3(2, 0, 0), 0.0) == "")
	state.players["a"]["protected_until"] = 0.0
	state.position(12, Vector3(15, 0, 0))
	var wall_id: int = state.buildings.keys()[0]
	_check("the owner does not hurt his own", state.building_damage(11, wall_id, 50.0, 1.0) == 0.0)
	_check("an enemy does", state.building_damage(12, wall_id, 200.0, 1.0) == 200.0)
	for second in range(2, 8):
		state.building_damage(12, wall_id, 250.0, float(second))
	_check("a wall is razed", not state.buildings.has(wall_id))
	_check("the snapshot has the buildings", state.snapshot(10.0)["buildings"].size() == state.buildings.size())
	state.leave(11, 20.0)
	_check("who leaves takes his buildings with him", state.buildings.is_empty())


func _blueprints() -> void:
	var state: RefCounted = MatchState.new()
	state.use_veins = false
	state.join("a", 11, "Anna", 0)
	state.place(11, Vector3(0, 0, 0), 0.0)
	_check("without its blueprint a wall is refused", state.build(11, "wall", Vector3(10, 0, 0), 0.0) != "")
	_check("the archive needs none", state.build(11, "archive", Vector3(-12, 0, 0), 0.0) == "")
	_check("three fragments are not enough", state.add_fragment(11, "wall") and state.add_fragment(11, "wall")
			and state.add_fragment(11, "wall") and state.research(11, "wall") != "")
	state.add_fragment(11, "wall")
	_check("four are, at one's archive", state.research(11, "wall") == "")
	_check("they are used up", int(state.progress["a"]["fragments"]["wall"]) == 0)
	_check("and the wall may be built", state.build(11, "wall", Vector3(10, 0, 0), 0.0) == "")
	_check("not twice", state.research(11, "wall") != "")
	var other: RefCounted = MatchState.new()
	other.use_veins = false
	other.join("b", 12, "Bert", 0)
	other.place(12, Vector3(0, 0, 0), 0.0)
	for n in 4:
		other.add_fragment(12, "ballista")
	_check("no research without an archive", other.research(12, "ballista") != "")
	state.leave(11, 5.0)
	state.join("a", 21, "Anna", 0)
	_check("what was found and learnt stays when one leaves", state.is_unlocked("a", "wall"))
	_check("the state tells each player his own", state.snapshot(6.0)["players"][0]["unlocked"].has("wall"))
	state.place(21, Vector3(0, 0, 0), 7.0)  # (leaving took his core and archive with him)
	state.build(21, "archive", Vector3(-12, 0, 0), 0.0)
	for n in 4:
		state.add_fragment(21, "frost")
	state.research(21, "frost")
	var spot: Vector2 = state.anchor_spots(Vector3.ZERO)[2]
	_check("a tower near an anchor stone goes onto it", state.build(21, "frost", Vector3(spot.x + 2.0, 0, spot.y - 2.0), 0.0) == ""
			and state.buildings.values().any(func(b: Dictionary) -> bool:
				return b["kind"] == "frost" and Vector2(b["x"], b["z"]).distance_to(spot) < 0.01))
	_check("a tower may also stand anywhere in the ring", state.build(21, "frost", Vector3(-15, 0, 10), 0.0) == "")


## Ticks `state` every half second from `from` to `to` (seconds); returns the events of that time.
func _run(state: RefCounted, from: float, to: float) -> Array:
	var events := []
	var now := from
	while now <= to + 0.001:
		state.tick(now)
		events.append_array(state.take_events())
		now += 0.5
	return events


func _count(events: Array, what: String) -> int:
	var count := 0
	for event: Dictionary in events:
		if event["what"] == what:
			count += 1
	return count


func _shrines() -> void:
	var state: RefCounted = MatchState.new()
	state.use_veins = false
	state.use_blueprints = false
	state.shrines.clear()
	state.add_shrine({"name": "Test", "kind": "stone", "x": 100.0, "z": 100.0, "region": "shroom"})
	state.add_shrine({"name": "Wood", "kind": "wood", "x": 300.0, "z": 300.0, "region": "wild"})
	for who: Array in [["a", 11, Vector3(0, 0, 0)], ["b", 12, Vector3(200, 0, 0)], ["c", 13, Vector3(400, 0, 0)]]:
		state.join(who[0], who[1], who[0], 0)
		state.place(who[1], who[2], 0.0)
	var at := Vector3(100, 0, 105)
	var away := Vector3(150, 0, 150)
	state.position(11, at)
	state.position(12, away)
	state.position(13, away)
	var events := _run(state, 0.0, 49.0)
	_check("a capture starts, and a monster comes every 2 s", _count(events, "shrine_capture") == 1
			and _count(events, "shrine_wave") == 24)
	_check("not taken before 50 s", state.shrine_owner(0) == "")
	events = _run(state, 49.5, 50.5)
	_check("taken after 50 s", state.shrine_owner(0) == "a" and _count(events, "shrine_taken") == 1)
	events = _run(state, 51.0, 60.0)
	_check("it gives its owner stone every 3 s (at 51, 54, 57 and 60 s)", int(state.progress["a"].get("stone", 0)) == 4)
	_check("and nobody wood", int(state.progress["a"].get("wood", 0)) == 0)
	# b tries to take it over; a stands in it: blocked.
	state.position(12, at)
	events = _run(state, 60.5, 85.0)
	_check("the owner inside blocks a takeover", state.shrine_owner(0) == "a" and state.shrines[0]["blocked"]
			and _count(events, "shrine_capture") == 0)
	state.position(11, away)
	events = _run(state, 85.5, 130.0)
	_check("with the owner gone b takes it over in 50 s", state.shrine_owner(0) == "" or state.shrine_owner(0) == "a")
	_check("... not yet at 44.5 s", state.shrine_owner(0) == "a")
	events = _run(state, 130.5, 136.0)
	_check("... and then", state.shrine_owner(0) == "b" and _count(events, "shrine_taken") == 1)
	# c starts and leaves at 19 s: reset.
	state.position(12, away)
	state.position(13, at)
	_run(state, 136.5, 155.0)
	state.position(13, away)
	events = _run(state, 155.5, 156.0)
	_check("leaving before the end cancels it", _count(events, "shrine_cancelled") == 1
			and float(state.shrines[0]["progress"]) == 0.0 and state.shrine_owner(0) == "b")
	# c dies during a capture.
	state.position(13, at)
	_run(state, 156.5, 160.0)
	state.position(13, at, false)
	events = _run(state, 160.5, 161.0)
	_check("dying cancels it", _count(events, "shrine_cancelled") == 1 and events.filter(
			func(e: Dictionary) -> bool: return e["what"] == "shrine_cancelled")[0]["why"] == "died")
	# A second player walks in during c's capture.
	state.position(13, at, true)
	_run(state, 161.5, 165.0)
	state.position(11, at)
	events = _run(state, 165.5, 166.0)
	_check("a second player in the radius blocks it", events.filter(
			func(e: Dictionary) -> bool: return e["what"] == "shrine_cancelled")[0]["why"] == "blocked"
			and state.shrines[0]["blocked"])
	# A player without a core cannot capture.
	state.position(11, away)
	state.position(13, away)
	state.players["c"]["core"] = null
	state.position(13, Vector3(300, 0, 300))
	_run(state, 166.5, 220.0)
	_check("without a core nobody captures", state.shrine_owner(1) == "")
	# b's core is destroyed: his shrine is neutral again.
	state.players["b"]["protected_until"] = 0.0
	state.players["a"]["position"] = Vector3(200, 0, 10)
	state.players["b"]["health"] = 100.0
	state.core_damage(11, "b", 150.0, 221.0)
	_check("a destroyed core frees its shrines", state.shrine_owner(0) == "")
	# One who leaves keeps his shrine but earns nothing.
	var keep: RefCounted = MatchState.new()
	keep.use_veins = false
	keep.free_build = false
	keep.shrines.clear()
	keep.add_shrine({"name": "W", "kind": "wood", "x": 0.0, "z": 0.0, "region": "wild"})
	keep.join("d", 21, "d", 0)
	keep.join("e", 22, "e", 0)
	keep.start(21, 0.0)
	keep.place(21, Vector3(100, 0, 0), 1.0)
	keep.place(22, Vector3(-100, 0, 0), 1.0)
	_run(keep, 1.0, 95.0)
	keep.position(21, Vector3(0, 0, 3))
	keep.position(22, Vector3(-100, 0, 0))
	_run(keep, 95.5, 146.0)
	_check("(rounds) d holds it", keep.shrine_owner(0) == "d")
	var before := int(keep.progress["d"].get("wood", 0))
	keep.leave(21, 147.0)
	_run(keep, 147.0, 160.0)
	_check("one who left keeps his shrine", keep.shrine_owner(0) == "d")
	_check("but earns nothing while away", int(keep.progress["d"].get("wood", 0)) == before)
	_check("the state tells each player his stone and wood", state.snapshot(131.0)["players"][0].has("stone")
			and state.snapshot(131.0)["shrines"].size() == 2)


func _wilds() -> void:
	var state: RefCounted = MatchState.new()
	state.use_veins = false
	state.use_blueprints = false
	state.shrines.clear()
	state.wilds_rules["seed"] = 7.0
	state.join("a", 11, "a", 0)
	state.join("b", 12, "b", 0)
	state.place(11, Vector3(0, 0, 0), 0.0)
	state.position(11, Vector3(0, 0, 0))
	state.position(12, Vector3(400, 0, 400))
	state.take_events()
	var events := _run(state, 0.5, 12.0)
	var packs: Array = events.filter(func(e: Dictionary) -> bool: return e["what"] == "wilds_pack")
	_check("two packs about each player", packs.size() == 4 and state.wilds.size() == 4)
	var well_placed := true
	var kinds := {}
	for pack: Dictionary in packs:
		var at := Vector2(float(pack["x"]), float(pack["z"]))
		var from_a := at.length()
		var from_b := at.distance_to(Vector2(400, 400))
		var nearest := minf(from_a, from_b)
		if nearest < 60.0 or nearest > 120.0 or from_a < 90.0:
			well_placed = false  # (a's core stands at 0: nothing within 90 m of it)
		if not MatchState.WILDS.has(pack["kind"]) or pack["slots"].is_empty() \
				or pack["slots"].size() < int(MatchState.WILDS[pack["kind"]]["pack"][0]) \
				or pack["slots"].size() > int(MatchState.WILDS[pack["kind"]]["pack"][1]):
			well_placed = false
		kinds[pack["kind"]] = true
	_check("60 to 120 m from a player, never within 90 m of a core, a pack's size of its kind", well_placed)
	_check("they are in the state", state.snapshot(12.0)["wilds"].size() == 4)
	_check("no more while each has his two", _count(_run(state, 12.5, 30.0), "wilds_pack") == 0)
	var first: int = packs[0]["id"]
	state.wilds_gone(first)
	_check("a pack gone frees its places and is told", not state.wilds.has(first)
			and _count(state.take_events(), "wilds_gone") == 1)
	_check("... and another comes", _count(_run(state, 30.5, 40.0), "wilds_pack") == 1)
	# The same seed gives the same packs on another server.
	var again: RefCounted = MatchState.new()
	again.use_veins = false
	again.use_blueprints = false
	again.shrines.clear()
	again.wilds_rules["seed"] = 7.0
	again.join("a", 11, "a", 0)
	again.join("b", 12, "b", 0)
	again.place(11, Vector3(0, 0, 0), 0.0)
	again.position(11, Vector3(0, 0, 0))
	again.position(12, Vector3(400, 0, 400))
	again.take_events()
	var again_packs: Array = _run(again, 0.5, 12.0).filter(func(e: Dictionary) -> bool: return e["what"] == "wilds_pack")
	_check("the same seed, the same packs", again_packs == packs)
	# Nothing in the dungeon, nothing at the festival, nothing when switched off, a cap.
	var deep: RefCounted = MatchState.new()
	deep.use_veins = false
	deep.use_blueprints = false
	deep.shrines.clear()
	deep.wilds_rules["seed"] = 3.0
	deep.join("c", 13, "c", 0)
	deep.position(13, Vector3(20, -180, 280))
	deep.take_events()
	_check("nothing over a player in the dungeon", _count(_run(deep, 0.5, 12.0), "wilds_pack") == 0)
	deep.position(13, Vector3(-23, 7, 216))  # 100 m east of the festival: packs come only on the far side
	var at_festival: Array = _run(deep, 12.5, 24.0).filter(func(e: Dictionary) -> bool: return e["what"] == "wilds_pack")
	var festival_free := true
	for pack: Dictionary in at_festival:
		if Vector2(float(pack["x"]), float(pack["z"])).distance_to(MatchState.FESTIVAL) < 140.0:
			festival_free = false
	_check("nothing within 140 m of the festival", at_festival.size() > 0 and festival_free)
	var off: RefCounted = MatchState.new()
	off.use_veins = false
	off.use_blueprints = false
	off.shrines.clear()
	off.wilds_rules["enabled"] = 0.0
	off.join("d", 14, "d", 0)
	off.position(14, Vector3(0, 0, 0))
	off.take_events()
	_check("switched off: nothing", _count(_run(off, 0.5, 12.0), "wilds_pack") == 0)
	var crowd: RefCounted = MatchState.new()
	crowd.use_veins = false
	crowd.use_blueprints = false
	crowd.shrines.clear()
	crowd.wilds_rules["seed"] = 5.0
	crowd.wilds_rules["packs_max"] = 3.0
	crowd.join("e", 15, "e", 0)
	crowd.join("f", 16, "f", 0)
	crowd.position(15, Vector3(0, 0, 0))
	crowd.position(16, Vector3(500, 0, 0))
	crowd.take_events()
	_check("packs_max caps them", _count(_run(crowd, 0.5, 30.0), "wilds_pack") == 3)
	_check("every kind has a pool of eight", MatchState.wild_pool_of("troll") == 8 and MatchState.WILDS.size() == MatchState.UNIT_KINDS.size())


func _units() -> void:
	var state: RefCounted = MatchState.new()
	state.use_veins = false
	state.use_blueprints = false
	state.shrines.clear()
	state.join("a", 11, "a", 0)
	state.join("b", 12, "b", 0)
	_check("no units without a core", state.make_unit(11, "wolf") != "")
	state.place(11, Vector3(0, 0, 0), 0.0)
	state.place(12, Vector3(200, 0, 0), 0.0)
	_check("no units without a Soul Forge", state.make_unit(11, "wolf") != "")
	state.build(11, "forge", Vector3(10, 0, 10), 0.0)
	state.take_events()
	_check("a wolf at the forge", state.make_unit(11, "wolf") == "")
	var made: Array = state.take_events()
	_check("... told with its place in the pool", made.size() == 1 and made[0]["what"] == "unit_made" and made[0]["slot"] == 0)
	for n in 7:
		state.make_unit(11, "troll")
	_check("eight at most", state.make_unit(11, "spider") != "" and state.count_units("a") == 8)
	_check("each in its own place of its kind", state.units.values().filter(
			func(u: Dictionary) -> bool: return u["kind"] == "troll").map(func(u: Dictionary) -> int: return u["slot"]) == [0, 1, 2, 3, 4, 5, 6])
	_check("nothing of an unknown kind", state.make_unit(12, "dragon") != "")
	# Every monster of the island (2026-10-22): each kind names its family's soul and its own pool.
	_check("twenty kinds", MatchState.UNIT_KINDS.size() == 20)
	_check("a kind's family", MatchState.unit_family_of("spider_crystal") == "spider" and MatchState.unit_family_of("wolf") == "wolf")
	_check("a kind's pool", MatchState.unit_pool_of("troll") == 8 and MatchState.unit_pool_of("undead") == 6)
	var pools: RefCounted = MatchState.new()
	pools.use_veins = false
	pools.use_blueprints = false
	pools.shrines.clear()
	pools.join("c", 13, "c", 0)
	pools.place(13, Vector3(0, 0, 0), 0.0)
	pools.build(13, "forge", Vector3(10, 0, 10), 0.0)
	for n in 6:
		pools.make_unit(13, "undead")
	_check("a kind's pool is its own", pools.count_units("c") == 6 and pools.make_unit(13, "undead").begins_with("No more")
			and pools.make_unit(13, "wolf") == "")
	_check("orders", state.set_order(11, "attack") == "" and state.set_order(11, "dance") != ""
			and state.snapshot(1.0)["players"][0]["order"] == "attack")
	var first: int = state.units.keys()[0]
	state.unit_died(12, first)
	_check("a unit that fell frees its place", state.count_units("a") == 7)
	# Units strike b's core: counted, no reach check; not while protected.
	state.players["b"]["protected_until"] = 0.0
	var before: float = state.players["b"]["health"]
	_check("a unit's blow on a core counts", state.unit_strike(12, "a", "core", "b", 30.0, 5.0) == 30.0
			and state.players["b"]["health"] == before - 30.0)
	_check("only for an owner with units", state.unit_strike(11, "b", "core", "a", 30.0, 5.0) == 0.0)
	state.players["b"]["protected_until"] = 100.0
	_check("not while the core is protected", state.unit_strike(12, "a", "core", "b", 30.0, 6.0) == 0.0)
	state.build(12, "wall", Vector3(210, 0, 10), 0.0)
	state.players["b"]["protected_until"] = 0.0
	var wall: int = state.buildings.keys().filter(func(id: int) -> bool: return state.buildings[id]["owner"] == "b")[0]
	_check("nor a wall", state.unit_strike(12, "a", "building", wall, 40.0, 7.0) == 40.0)
	state.take_events()
	# a's core is destroyed: his units crumble.
	state.players["a"]["protected_until"] = 0.0
	state.players["b"]["position"] = Vector3(0, 0, 5)
	state.players["a"]["health"] = 10.0
	state.core_damage(12, "a", 50.0, 8.0)
	_check("a lost core: the units crumble", state.count_units("a") == 0 and state.take_events().any(
			func(e: Dictionary) -> bool: return e["what"] == "units_lost"))
