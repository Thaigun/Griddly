#pragma once

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "Actions/Action.hpp"
#include "Objects/GridLocation.hpp"

namespace griddy {

class Grid {
 public:
  Grid(int width, int height);
  ~Grid();

  void cloneState() const;
  void update(std::vector<std::shared_ptr<Action>> actions);

  int getCurrentScore(int playerId) const;
  int getResources(int playerId) const;

  int getWidth() const;
  int getHeight() const;

  void initObject(GridLocation location, std::shared_ptr<Object> object);
  std::unordered_set<std::shared_ptr<Object>>& getObjects();

  std::shared_ptr<Object> getObject(GridLocation location) const;

 private:
  const int height_;
  const int width_;

  std::unordered_set<std::shared_ptr<Object>> objects_;
  std::unordered_map<GridLocation, std::shared_ptr<Object>, GridLocation::Hash> occupiedLocations_;
};

}  // namespace griddy