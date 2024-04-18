import numpy as np
import pygambit as gbt
import pytest

from . import games


def test_mixed_outcome_to_array():
    g = gbt.Game.new_table([2, 2])
    g.outcomes[0][g.players[0]] = "1/4"
    g.outcomes[0][g.players[1]] = 0.25
    g.outcomes[1][g.players[0]] = 5
    g.outcomes[1][g.players[1]] = 0.
    g.outcomes[2][g.players[0]] = 0
    g.outcomes[2][g.players[1]] = 5/1
    g.outcomes[3][g.players[0]] = "3.14159265"
    g.outcomes[3][g.players[1]] = 3

    A, B = g.to_arrays()
    A_true = np.array([[0.25, 0.], [5., 3.14159265]])
    B_true = np.array([[0.25, 5.], [0., 3.]])

    assert np.all(A_true == A)
    assert np.all(B_true == B)


def test_from_empty_array_to_array():
    game = gbt.Game.from_arrays([])
    a = game.to_arrays()
    assert len(a) == 1
    assert a[0].size == 0


def test_empty_game_to_array():
    game = gbt.Game.new_table([])
    assert game.to_arrays() == []


def test_uninitialised_game_to_array():
    game = gbt.Game()
    with pytest.raises(RuntimeError):
        _ = game.to_arrays()


def test_ext_form_to_arrays():
    game = gbt.Game.new_tree()
    with pytest.raises(ValueError):
        _ = game.to_arrays()


def test_from_arrays_to_arrays():
    A = np.array([[1, 5], [0, 3]])
    B = A.T
    game = gbt.Game.from_arrays(A, B)
    a, b = game.to_arrays()

    assert np.all(a == A)
    assert np.all(b == B)


def test_from_arrays():
    m = np.array([[8, 2], [10, 5]])
    game = gbt.Game.from_arrays(m, m.transpose())
    assert len(game.players) == 2
    assert len(game.players[0].strategies) == 2
    assert len(game.players[1].strategies) == 2


def test_from_dict():
    m = np.array([[8, 2], [10, 5]])
    game = gbt.Game.from_dict({"a": m, "b": m.transpose()})
    assert len(game.players) == 2
    assert len(game.players[0].strategies) == 2
    assert len(game.players[1].strategies) == 2
    assert game.players[0].label == "a"
    assert game.players[1].label == "b"


def test_game_get_outcome_by_index():
    game = gbt.Game.new_table([2, 2])
    assert game[0, 0] == game.outcomes[0]


def test_game_get_outcome_by_label():
    game = gbt.Game.new_table([2, 2])
    game.players[0].strategies[0].label = "defect"
    game.players[1].strategies[0].label = "cooperate"
    assert game["defect", "cooperate"] == game.outcomes[0]


def test_game_get_outcome_invalid_tuple_size():
    game = gbt.Game.new_table([2, 2])
    with pytest.raises(KeyError):
        _ = game[0, 0, 0]


def test_game_outcomes_non_tuple():
    game = gbt.Game.new_table([2, 2])
    with pytest.raises(TypeError):
        _ = game[42]


def test_game_outcomes_type_exception():
    game = gbt.Game.new_table([2, 2])
    with pytest.raises(TypeError):
        _ = game[1.23, 1]


def test_game_get_outcome_index_out_of_range():
    game = gbt.Game.new_table([2, 2])
    with pytest.raises(IndexError):
        _ = game[0, 3]


def test_game_get_outcome_unmatched_label():
    game = gbt.Game.new_table([2, 2])
    game.players[0].strategies[0].label = "defect"
    game.players[1].strategies[0].label = "cooperate"
    with pytest.raises(IndexError):
        _ = game["defect", "defect"]


def test_game_get_outcome_with_strategies():
    game = gbt.Game.new_table([2, 2])
    assert (
        game[game.players[0].strategies[0], game.players[1].strategies[0]] ==
        game.outcomes[0]
    )


def test_game_get_outcome_with_bad_strategies():
    game = gbt.Game.new_table([2, 2])
    with pytest.raises(IndexError):
        _ = game[game.players[0].strategies[0], game.players[0].strategies[0]]


def test_game_dereference_invalid():
    game = gbt.Game.new_tree()
    game.add_player("One")
    strategy = game.players[0].strategies[0]
    game.append_move(game.root, game.players[0], ["a", "b"])
    with pytest.raises(RuntimeError):
        _ = strategy.label


def test_strategy_profile_invalidation_table():
    """Test for invalidating mixed strategy profiles on tables when game changes.
    """
    g = gbt.Game.new_table([2, 2])
    profiles = [g.mixed_strategy_profile(rational=b) for b in [False, True]]
    g.delete_strategy(g.players[0].strategies[0])
    for profile in profiles:
        with pytest.raises(gbt.GameStructureChangedError):
            profile.payoff(g.players[0])
        with pytest.raises(gbt.GameStructureChangedError):
            profile.liap_value()


def test_strategy_profile_invalidation_payoff():
    g = gbt.Game.from_arrays([[2, 2], [0, 0]], [[0, 0], [1, 1]])
    profiles = [g.mixed_strategy_profile(rational=b) for b in [False, True]]
    g.outcomes[0][g.players[0]] = 3
    for profile in profiles:
        with pytest.raises(gbt.GameStructureChangedError):
            profile.payoff(g.players[0])
        with pytest.raises(gbt.GameStructureChangedError):
            profile.liap_value()


def test_behavior_profile_invalidation():
    """Test for invalidating mixed strategy profiles on tables when game changes.
    """
    g = games.read_from_file("basic_extensive_game.efg")
    profiles = [g.mixed_behavior_profile(rational=b) for b in [False, True]]
    g.delete_action(g.players[0].infosets[0].actions[0])
    for profile in profiles:
        with pytest.raises(gbt.GameStructureChangedError):
            profile.payoff(g.players[0])
        with pytest.raises(gbt.GameStructureChangedError):
            profile.liap_value()
