#pragma once

#include <glm/glm.hpp>
#include <string>

#include "../../Grid.hpp"
#include "../Objects/Object.hpp"
#include "Direction.hpp"

namespace griddly {

class Grid;

// How the action is initialized so destination and sources can be retrieved
enum class ActionMode {
  SRC_LOC_DST_LOC,
  SRC_OBJ_DST_LOC,
  SRC_OBJ_DST_OBJ,
  SRC_OBJ_DST_VEC
};

struct InputMapping {
  glm::ivec2 vectorToDest{};
  glm::ivec2 orientationVector{};
  std::string description = "";
  std::unordered_map<std::string, int32_t> metaData{};
};

struct ActionInputsDefinition {
  std::unordered_map<uint32_t, InputMapping> inputMappings{};
  bool relative = false;
  bool internal = false;
  bool mapToGrid = false;
};

class Action {
 public:
  Action(std::shared_ptr<Grid> grid, std::string actionName, uint32_t playerId, uint32_t delay = 0, std::unordered_map<std::string, int32_t> metaData = {});

  // An action that is tied to specific objects, used in triggered actions
  virtual void init(std::shared_ptr<Object> sourceObject, std::shared_ptr<Object> destinationObject);

  // An action that is not tied to any specific units in the grid, these actions can be performed by the environment, or can be RTS input
  virtual void init(glm::ivec2 sourceLocation, glm::ivec2 destinationLocation);

  // Action used to specify a source object and a direction for an action. Can be used for actions that are relative to avatars.
  virtual void init(std::shared_ptr<Object> sourceObject, glm::ivec2 vectorToDest, glm::ivec2 orientationVector, bool relativeToSource);

  // resolve the source object in the current grid
  virtual std::shared_ptr<Object> getSourceObject() const;

  // resolve the destination object in the current grid
  virtual std::shared_ptr<Object> getDestinationObject() const;

  virtual glm::ivec2 getSourceLocation() const;

  virtual glm::ivec2 getDestinationLocation() const;

  virtual glm::ivec2 getVectorToDest() const;

  virtual std::string getActionName() const;

  virtual std::string getDescription() const;

  virtual glm::ivec2 getOrientationVector() const;

  virtual uint32_t getOriginatingPlayerId() const;

  // Delay an action
  virtual uint32_t getDelay() const;

  virtual std::unordered_map<std::string, int32_t> getMetaData() const;

  virtual int32_t getMetaData(std::string variableName) const;

  virtual ~Action() = default;

 protected:
  std::shared_ptr<Object> sourceObject_ ;
  std::shared_ptr<Object> destinationObject_;
  glm::ivec2 destinationLocation_ = {0, 0};
  glm::ivec2 sourceLocation_ = {0, 0};
  glm::ivec2 vectorToDest_ = {0, 0};

  glm::ivec2 orientationVector_ = {0, 0};

  const std::string actionName_;
  const uint32_t delay_;
  const std::weak_ptr<Grid> grid_;
  const uint32_t playerId_ = 0;

  // Some variables that we can set in the input mapping
  const std::unordered_map<std::string, int32_t> metaData_;

 private:
  ActionMode actionMode_;

  std::shared_ptr<Grid> grid() const;

};
}  // namespace griddly