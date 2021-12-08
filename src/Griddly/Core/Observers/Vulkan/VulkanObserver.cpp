#include "VulkanObserver.hpp"

#include <spdlog/spdlog.h>

#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtx/color_space.hpp>

#include "VulkanConfiguration.hpp"
#include "VulkanDevice.hpp"
#include "VulkanInstance.hpp"

namespace griddly {

std::shared_ptr<vk::VulkanInstance> VulkanObserver::instance_ = nullptr;

VulkanObserver::VulkanObserver(std::shared_ptr<Grid> grid, ResourceConfig resourceConfig, ShaderVariableConfig shaderVariableConfig) : Observer(grid), resourceConfig_(resourceConfig), shaderVariableConfig_(shaderVariableConfig) {
}

VulkanObserver::~VulkanObserver() {
  spdlog::debug("VulkanObserver Destroyed");
}

void VulkanObserver::init(ObserverConfig observerConfig) {
  Observer::init(observerConfig);
  uint32_t players = grid_->getPlayerCount();

  float s = 1.0f;
  float v = 0.6f;
  float h_inc = 360.0f / players;
  for (uint32_t p = 0; p < players; p++) {
    uint32_t h = h_inc * p;
    glm::vec4 rgba = glm::vec4(glm::rgbColor(glm::vec3(h, s, v)), 1.0);
    playerColors_.push_back(rgba);
  }
}

/**
 * Only load vulkan if update() called, allows many environments with vulkan-based global observers to be used. 
 * But only loads them if global observations are requested, for example for creating videos
 * 
 * This a) allows significantly more enviroments to be loaded (if only one of them is being used to create videos) and b) 
 */
void VulkanObserver::lazyInit() {
  if (observerState_ != ObserverState::RESET) {
    throw std::runtime_error("Cannot initialize Vulkan Observer when it is not in RESET state.");
  }

  spdlog::debug("Vulkan lazy initialization....");

  gridBoundary_ = glm::ivec2(grid_->getWidth(), grid_->getHeight());

  auto imagePath = resourceConfig_.imagePath;
  auto shaderPath = resourceConfig_.shaderPath;

  auto configuration = vk::VulkanConfiguration();
  if (instance_ == nullptr) {
    instance_ = std::shared_ptr<vk::VulkanInstance>(new vk::VulkanInstance(configuration));
  }

  device_ = std::make_shared<vk::VulkanDevice>(vk::VulkanDevice(instance_, observerConfig_.tileSize, shaderPath));
  device_->initDevice(false);

  // This is probably far too big for most circumstances, but not sure how to work this one out in a smarter way,
  const int maxObjects = 100000;

  device_->initializeSSBOs(
      shaderVariableConfig_.exposedGlobalVariables.size(),
      grid_->getPlayerCount(),
      shaderVariableConfig_.exposedObjectVariables.size(),
      maxObjects);

  observerState_ = ObserverState::READY;
}

void VulkanObserver::reset() {
  Observer::reset();

  shouldUpdateCommandBuffer_ = true;

  if (observerState_ == ObserverState::READY) {
    resetRenderSurface();
  }
}

vk::PersistentSSBOData VulkanObserver::updatePersistentShaderBuffers() {
  spdlog::debug("Updating persistent shader buffers.");
  vk::PersistentSSBOData persistentSSBOData;

  for (int p = 0; p < grid_->getPlayerCount(); p++) {
    vk::PlayerInfoSSBO playerInfo;
    playerInfo.playerColor = playerColors_[p];
    persistentSSBOData.playerInfoSSBOData.push_back(playerInfo);
  }


  spdlog::debug("Highlighting players {0}", observerConfig_.highlightPlayers ? "true": "false");

  persistentSSBOData.environmentUniform.viewMatrix = getViewMatrix();
  persistentSSBOData.environmentUniform.gridDims = glm::vec2{gridWidth_, gridHeight_};
  persistentSSBOData.environmentUniform.highlightPlayerObjects = observerConfig_.highlightPlayers ? 1 : 0;
  persistentSSBOData.environmentUniform.playerId = observerConfig_.playerId;
  persistentSSBOData.environmentUniform.projectionMatrix = glm::ortho(0.0f, static_cast<float>(pixelWidth_), 0.0f, static_cast<float>(pixelHeight_));
  persistentSSBOData.environmentUniform.globalVariableCount = shaderVariableConfig_.exposedGlobalVariables.size();
  persistentSSBOData.environmentUniform.objectVariableCount = shaderVariableConfig_.exposedObjectVariables.size();

  return persistentSSBOData;
}

uint8_t* VulkanObserver::update() {
  if (observerState_ == ObserverState::RESET) {
    lazyInit();
    resetRenderSurface();
  } else if (observerState_ != ObserverState::READY) {
    throw std::runtime_error("Observer is not in READY state, cannot render");
  }

  auto frameSSBOData = updateFrameShaderBuffers();
  device_->writeFrameSSBOData(frameSSBOData);

  if (shouldUpdateCommandBuffer_) {
    device_->startRecordingCommandBuffer();
    updateCommandBuffer(frameSSBOData.objectDataSSBOData);
    device_->endRecordingCommandBuffer(std::vector<VkRect2D>{{{0, 0}, {pixelWidth_, pixelHeight_}}});
    shouldUpdateCommandBuffer_ = false;
  }

  grid_->purgeUpdatedLocations(observerConfig_.playerId);

  return device_->renderFrame();
}

void VulkanObserver::resetRenderSurface() {
  spdlog::debug("Initializing Render Surface. Grid width={0}, height={1}. Pixel width={2}. height={3}", gridWidth_, gridHeight_, pixelWidth_, pixelHeight_);
  observationStrides_ = device_->resetRenderSurface(pixelWidth_, pixelHeight_);

  auto persistentSSBOData = updatePersistentShaderBuffers();
  device_->writePersistentSSBOData(persistentSSBOData);
}

void VulkanObserver::release() {
  device_.reset();
}

void VulkanObserver::print(std::shared_ptr<uint8_t> observation) {
  auto tileSize = observerConfig_.tileSize;
  std::string filename = fmt::format("{0}.ppm", *grid_->getTickCount());
  std::ofstream file(filename, std::ios::out | std::ios::binary);

  auto width = grid_->getWidth() * tileSize.x;
  auto height = grid_->getHeight() * tileSize.y;

  // ppm header
  file << "P6\n"
       << width << "\n"
       << height << "\n"
       << 255 << "\n";

  file.write((char*)observation.get(), width * height * 3);
  file.close();
}

}  // namespace griddly