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
	_check("not off the island", state.place(13, Vector3(900, 0, 0)) != "")
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
