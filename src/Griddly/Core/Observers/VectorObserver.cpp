#include "VectorObserver.hpp"

#define SPDLOG_HEADER_ONLY
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include <memory>

namespace griddly {

VectorObserver::VectorObserver(std::shared_ptr<Grid> grid) : Observer(grid) {}

void VectorObserver::init(VectorObserverConfig& config) {
  Observer::init(config);
  config_ = config;
}

const VectorObserverConfig& VectorObserver::getConfig() const {
  return config_;
}

void VectorObserver::reset() {
  Observer::reset();

  // there are no additional steps until this observer can be used.
  observerState_ = ObserverState::READY;
}

ObserverType VectorObserver::getObserverType() const {
  return ObserverType::VECTOR;
}

void VectorObserver::resetShape() {
  auto config = getConfig();
  gridWidth_ = config.overrideGridWidth > 0 ? config.overrideGridWidth : grid_->getWidth();
  gridHeight_ = config.overrideGridHeight > 0 ? config.overrideGridHeight : grid_->getHeight();

  gridBoundary_.x = grid_->getWidth();
  gridBoundary_.y = grid_->getHeight();

  observationChannels_ = static_cast<uint32_t>(grid_->getObjectIds().size());

  // Always in order objects, player, orientation, variables.
  if (config.includePlayerId) {
    channelsBeforePlayerCount_ = observationChannels_;
    observationChannels_ += config.playerCount + 1;  // additional one-hot for "no-player"

    spdlog::debug("Adding {0} playerId channels at: {1}", observationChannels_ - channelsBeforePlayerCount_, channelsBeforePlayerCount_);
  }

  if (config.includeRotation) {
    channelsBeforeRotation_ = observationChannels_;
    observationChannels_ += 4;
    spdlog::debug("Adding {0} rotation channels at: {1}", observationChannels_ - channelsBeforeRotation_, channelsBeforeRotation_);
  }

  if (config.includeVariables) {
    channelsBeforeVariables_ = observationChannels_;
    observationChannels_ += static_cast<uint32_t>(grid_->getObjectVariableIds().size());
    spdlog::debug("Adding {0} variable channels at: {1}", observationChannels_ - channelsBeforeVariables_, channelsBeforeVariables_);
  }

  observationShape_ = {observationChannels_, gridWidth_, gridHeight_};
  observationStrides_ = {1, observationChannels_, observationChannels_ * gridWidth_};

  observation_ = std::shared_ptr<uint8_t>(new uint8_t[observationChannels_ * gridWidth_ * gridHeight_]{}); //NOLINT

}

void VectorObserver::renderLocation(glm::ivec2 objectLocation, glm::ivec2 outputLocation, bool resetLocation) const {
  auto config = getConfig();
  auto memPtr = observation_.get() + observationChannels_ * (gridWidth_ * outputLocation.y + outputLocation.x);

  if (resetLocation) {
    auto size = sizeof(uint8_t) * observationChannels_;
    memset(memPtr, 0, size);
  }

  // Only put the *include* information of the first object
  bool processTopLayer = true;
  for (auto& objectIt : grid_->getObjectsAt(objectLocation)) {
    auto object = objectIt.second;
    auto objectName = object->getObjectName();
    spdlog::debug("Rendering object {0}", objectName);
    auto memPtrObject = memPtr + grid_->getObjectIds().at(objectName);
    *memPtrObject = 1;

    if (processTopLayer) {
      if (config.includePlayerId) {
        auto playerIdx = getEgocentricPlayerId(object->getPlayerId());

        auto playerMemPtr = memPtr + channelsBeforePlayerCount_ + playerIdx;
        *playerMemPtr = 1;
      }

      if (config.includeRotation) {
        uint32_t directionIdx = 0;
        switch (object->getObjectOrientation().getDirection()) {
          case Direction::UP:
          case Direction::NONE:
            directionIdx = 0;
            break;
          case Direction::RIGHT:
            directionIdx = 1;
            break;
          case Direction::DOWN:
            directionIdx = 2;
            break;
          case Direction::LEFT:
            directionIdx = 3;
            break;
        }
        auto orientationMemPtr = memPtr + channelsBeforeRotation_ + directionIdx;
        *orientationMemPtr = 1;
      }

      if (config.includeVariables) {
        for (auto& variableIt : object->getAvailableVariables()) {
          auto variableValue = *variableIt.second;
          auto variableName = variableIt.first;

          // If the variable is one of the variables defined in the object, get the index of the variable and set it to the variable's value
          auto objectVariableIt = grid_->getObjectVariableIds().find(variableName);
          if (objectVariableIt != grid_->getObjectVariableIds().end()) {
            uint32_t variableIdx = objectVariableIt->second;

            auto variableMemPtr = memPtr + channelsBeforeVariables_ + variableIdx;
            *variableMemPtr = variableValue;
          }
        }
      }

      processTopLayer = false;
    }
  }
}

uint8_t& VectorObserver::update() {
  auto config = getConfig();
  spdlog::debug("Vector renderer updating.");

  if (observerState_ != ObserverState::READY) {
    throw std::runtime_error("Observer not ready, must be initialized and reset before update() can be called.");
  }

  if (doTrackAvatar_) {
    spdlog::debug("Tracking Avatar.");

    auto avatarLocation = avatarObject_->getLocation();
    auto avatarOrientation = avatarObject_->getObjectOrientation();
    auto avatarDirection = avatarOrientation.getDirection();

    // Have to reset the observation
    auto size = sizeof(uint8_t) * observationChannels_ * gridWidth_ * gridHeight_;
    memset(observation_.get(), 0, size);

    if (config.rotateWithAvatar) {
      // Assuming here that gridWidth and gridHeight are odd numbers
      auto pGrid = getAvatarObservableGrid(avatarLocation, avatarDirection);
      uint32_t outx = 0, outy = 0;
      switch (avatarDirection) {
        default:
        case Direction::UP:
        case Direction::NONE:
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outy = 0;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              renderLocation({objx, objy}, {outx, outy});
              outy++;
            }
            outx++;
          }
          break;
        case Direction::DOWN:
          outx = gridWidth_ - 1;
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outy = gridHeight_ - 1;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              renderLocation({objx, objy}, {outx, outy});
              outy--;
            }
            outx--;
          }
          break;
        case Direction::RIGHT:
          outy = gridHeight_ - 1;
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outx = 0;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              renderLocation({objx, objy}, {outx, outy});
              outx++;
            }
            outy--;
          }
          break;
        case Direction::LEFT:
          for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
            outx = gridWidth_ - 1;
            for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
              renderLocation({objx, objy}, {outx, outy});
              outx--;
            }
            outy++;
          }
          break;
      }

    } else {
      auto pGrid = getAvatarObservableGrid(avatarLocation, Direction::NONE);

      uint32_t outx = 0, outy = 0;
      for (auto objx = pGrid.left; objx <= pGrid.right; objx++) {
        outy = 0;
        for (auto objy = pGrid.bottom; objy <= pGrid.top; objy++) {
          if (objx < gridBoundary_.x && objx >= 0 && objy < gridBoundary_.y && objy >= 0) {
            renderLocation({objx, objy}, {outx, outy});
          }
          outy++;
        }
        outx++;
      }
    }
  } else {
    const auto& updatedLocations = grid_->getUpdatedLocations(config.playerId);

    for (auto& location : updatedLocations) {
      if (location.x >= config.gridXOffset &&
          location.x < gridWidth_ + config.gridXOffset &&
          location.y >= config.gridYOffset &&
          location.y < gridHeight_ + config.gridYOffset) {
        auto outputLocation = glm::ivec2(
            location.x - config.gridXOffset,
            location.y - config.gridYOffset);

        spdlog::debug("Rendering location {0}, {1}.", location.x, location.y);

        if (outputLocation.x < gridWidth_ && outputLocation.x >= 0 && outputLocation.y < gridHeight_ && outputLocation.y >= 0) {
          renderLocation(location, outputLocation, true);
        }
      }
    }
  }

  spdlog::debug("Purging update locations.");

  grid_->purgeUpdatedLocations(config.playerId);

  spdlog::debug("Vector renderer done.");

  return *observation_.get();
}

}  // namespace griddly
