#ifndef CEE_ENGINE_DEBUG_LAYER_H
#define CEE_ENGINE_DEBUG_LAYER_H

#include <CeeEngine/layer.h>
#include <CeeEngine/messageBus.h>


namespace cee {
	class DebugLayer : public Layer {
	public:
		DebugLayer();
		~DebugLayer();

		void OnAttach() override;
		void OnDetach() override;

		void OnUpdate(Timestep t) override;
		void OnRender() override;
		void OnGui() override;

		void OnEnable() override;
		void OnDisable() override;

	protected:
		void MessageHandler(Event& e) override;
	};
}

#endif
