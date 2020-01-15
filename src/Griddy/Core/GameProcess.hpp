#pragma once

#include <memory>
#include <vector>
#include "Grid.hpp"
#include "Observers/Observer.hpp"

namespace griddy {

class Player;

class GameProcess {
 public:
  GameProcess(std::vector<std::shared_ptr<Player>> players, std::shared_ptr<Observer> observer, std::shared_ptr<Grid> grid);

  virtual std::unique_ptr<uint8_t[]> observe(std::shared_ptr<Player> player) const = 0;

  virtual void startGame() const = 0;
  virtual void endGame() const = 0;

  virtual std::string getProcessName() const;

  virtual ~GameProcess() = 0;

 private:
  std::vector<std::shared_ptr<Player>> players_;
  std::shared_ptr<Grid> grid_;
  std::shared_ptr<Observer> observer_;
};
}  // namespace griddy