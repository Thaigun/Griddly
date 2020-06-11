import numpy as np
from timeit import default_timer as timer

# The griddy lib is in the build directory when built so add it and then import
from griddly import gd, griddly_loader
from griddly.RenderTools import RenderWindow

window = None

if __name__ == '__main__':

    width = 30
    height = 30

    renderWindow = RenderWindow(32 * width, 32 * height)

    loader = griddly_loader()

    game_description = loader.load_game_description('RTS/basicRTS.yaml')

    grid = game_description.load_level(1)

    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    # Create a player
    player1 = game.add_player('Bob', gd.ObserverType.SPRITE_2D)
    player2 = game.add_player('Alice', gd.ObserverType.NONE)

    game.init()

    game.reset()

    start = timer()

    frames = 0

    # Player objects have the same interface as gym environments
    for i in range(0, 100000):
        for j in range(0, 10000):
            x = np.random.randint(width)
            y = np.random.randint(height)
            dir = np.random.randint(4)

            reward, done = player1.step("move", [x, y, dir])
            # reward = player2.step(x, y, gd.ActionType.MOVE, gd.Direction.LEFT)

            #player1_tiles = player1.observe()

            observation = np.array(game.observe(), copy=False)
            renderWindow.render(observation)

            frames += 1

            if frames % 1000 == 0:
                end = timer()
                print(f'fps: {frames / (end - start)}')
                frames = 0
                start = timer()

        game.reset()

    # player1.render()

    # player2.step()
    # player2.render()
    #
    # game.render()

    # grid.add_object(player1, gd., 0, 0)

    #
    # # Add an object at a particular location, the object might be owned by a player
    # grid.add_object(player, 'OBJECT_TYPE', 0, 0)
    #
    # # Create an action belonging to a plyer
    # #action = player.create_action(...)
    #
    # # Create an observer
    # observer = gd.create_observer('thing')
    #
    # # Create a game from the previous settings
    # game = gd.create_game_process([player], observer, grid)
    #
    # game.perform_actions([action])
