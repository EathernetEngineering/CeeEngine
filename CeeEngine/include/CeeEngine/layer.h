#ifndef CEE_ENGINE_LAYER_H
#define CEE_ENGINE_LAYER_H

#include <CeeEngine/platform.h>
#include <CeeEngine/event.h>
#include <CeeEngine/timestep.h>
#include <CeeEngine/messageBus.h>

#include <vector>

namespace cee {
	class CEEAPI LayerStack;

	class CEEAPI Layer {
	public:
		Layer() = default;
		virtual ~Layer() = default;

		virtual void OnAttach() = 0;
		virtual void OnDetach() = 0;

		virtual void OnUpdate(Timestep t) = 0;
		virtual void OnRender() = 0;
		virtual void OnGui() = 0;

		virtual void OnEnable() = 0;
		virtual void OnDisable() = 0;

		virtual void MessageHandler(Event& e) = 0;

	protected:
		MessageBus* m_MessageBus;
		friend LayerStack;
	};

	class CEEAPI LayerStack {
	public:
		typedef std::vector<Layer*>::iterator iterator;
		typedef std::vector<Layer*>::const_iterator const_iterator;
		typedef std::vector<Layer*>::reverse_iterator reverse_iterator;
		typedef std::vector<Layer*>::const_reverse_iterator const const_reverse_iterator;

	public:
		LayerStack(MessageBus* messageBus);
		~LayerStack() = default;

		void PushLayer(Layer* layer);
		void PopLayer(Layer* layer);

		void PushOverlay(Layer* overlay);
		void PopOverlay(Layer* overlay);

		iterator begin();
		const_iterator cbegin() const;

		iterator end();
		const_iterator cend() const;

		reverse_iterator rbegin();
		const_reverse_iterator crbegin() const;

		reverse_iterator rend();
		const_reverse_iterator crend() const;

	private:
		std::vector<Layer*> m_Layers;
		uint32_t m_LayerOffset;

		MessageBus* m_MessageBus;
	};
}

#endif
