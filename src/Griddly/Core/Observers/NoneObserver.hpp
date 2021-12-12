#include "../Grid.hpp"
#include "Observer.hpp"

namespace griddly {

class NoneObserver : public Observer {
 public:
  NoneObserver(std::shared_ptr<Grid> grid);
  ~NoneObserver() override = default;

  uint8_t* update() override;
  void resetShape() override;

  ObserverType getObserverType() const override;

 private:
  std::shared_ptr<uint8_t> emptyObs_;
};

}  // namespace griddly