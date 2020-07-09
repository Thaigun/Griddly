import numpy as np
from timeit import default_timer as timer

from griddly import gd, GriddlyLoader
from griddly.RenderTools import RenderWindow

window = None

if __name__ == '__main__':

    loader = GriddlyLoader()

    grid = loader.load_game_description('Single-Player/Mini-Grid/minigrid-doggo.yaml')

    grid.load_level(1)
    game = grid.create_game(gd.ObserverType.SPRITE_2D)

    # Create a player
    player1 = game.register_player('Bob', gd.ObserverType.VECTOR)

    game.init()

    game.reset()

    width = grid.get_width()
    height = grid.get_height()

    renderWindow = RenderWindow(70 * width, 70 * height)

    observation = np.array(game.observe(), copy=False)

    #renderWindow.render(observation)

    start = timer()

    frames = 0

    # Player objects have the same interface as gym environments
    for l in range(0, 5):
        for j in range(0, 1000):
            x = np.random.randint(width)
            y = np.random.randint(height)
            dir = np.random.randint(5)

            reward, done = player1.step("move", [dir])

            #player1_tiles = player1.observe()

            observation = np.array(game.observe(), copy=False)
            renderWindow.render(observation)

            # if reward != 0:
            #     print(f'reward: {reward} done: {done}')

            frames += 1

            if frames % 1000 == 0:
                end = timer()
                print(f'fps: {frames / (end - start)}')
                frames = 0
                start = timer()

        grid.load_level(l)
        game.reset()
        observation = np.array(game.observe(), copy=False)
        renderWindow.render(observation)


