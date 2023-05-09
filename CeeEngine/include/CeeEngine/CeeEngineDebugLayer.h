#ifndef CEE_ENGINE_DEBUG_LAYER_H
#define CEE_ENGINE_DEBUG_LAYER_H

#include "CeeEngine/CeeEngineLayer.h"
#include <CeeEngine/CeeEngineMessageBus.h>


namespace cee {
	class DebugLayer : public Layer {
	public:
		DebugLayer();
		~DebugLayer();

		void OnAttach() override;
		void OnDetach() override;

		void OnUpdate(Timestep t) override;
		void OnGui() override;

		void OnEnable() override;
		void OnDisable() override;

	protected:
		void MessageHandler(Event& e) override;
	};
}

#endif
