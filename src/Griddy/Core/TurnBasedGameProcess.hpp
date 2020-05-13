#pragma once
#include <memory>
#include <vector>
#include "GameProcess.hpp"

namespace griddy {

class GDYFactory;

class TurnBasedGameProcess : public GameProcess {
 public:
  TurnBasedGameProcess(std::shared_ptr<Grid> grid, std::shared_ptr<Observer> observer, std::shared_ptr<GDYFactory> gdyFactory);
  ~TurnBasedGameProcess();

  virtual std::vector<int> performActions(uint32_t playerId, std::vector<std::shared_ptr<Action>> actions) override;

  virtual std::string getProcessName() const override;

 private:
  static const std::string name_;

};
}  // namespace griddy