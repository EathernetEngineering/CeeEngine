#include <CeeEngine/layer.h>

#include <cstdio>
#include <algorithm>

namespace cee {

	LayerStack::LayerStack(MessageBus* messageBus)
	 : m_MessageBus(messageBus)
	{
	}

	void LayerStack::PushLayer(Layer* layer)
	{
		layer->m_MessageBus = m_MessageBus;
		layer->m_MessageBus->RegisterMessageHandler([layer](Event& e){ layer->MessageHandler(e); });
		m_Layers.emplace(m_Layers.begin() + m_LayerOffset, layer);
		layer->OnAttach();
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		iterator offset = std::find(m_Layers.begin(), m_Layers.end(), layer);
		if (offset == m_Layers.end()) {
			fprintf(stderr, "Warning: Overlay does not exist.");
			return;
		}
		if (offset < m_Layers.begin() + m_LayerOffset) {
			fprintf(stderr, "Warning: Using delete layer funtion on overlay.");
		}
		m_Layers.erase(offset);
		layer->OnDetach();
	}

	void LayerStack::PushOverlay(Layer* overlay)
	{
		overlay->m_MessageBus = m_MessageBus;
		overlay->m_MessageBus->RegisterMessageHandler([overlay](Event& e){ overlay->MessageHandler(e); });
		m_Layers.emplace(m_Layers.begin(), overlay);
		overlay->OnAttach();
		m_LayerOffset++;
	}

	void LayerStack::PopOverlay(Layer* overlay)
	{
		iterator offset = std::find(m_Layers.begin(), m_Layers.end(), overlay);
		if (offset == m_Layers.end()) {
			fprintf(stderr, "Warning: Overlay does not exist.");
			return;
		}
		if (offset >= m_Layers.begin() + m_LayerOffset) {
			m_Layers.erase(offset);
			fprintf(stderr, "Warning: Using delete overlay funtion on layer.");
			return;
		}
		m_Layers.erase(offset);
		overlay->OnDetach();
		m_LayerOffset--;
	}

	LayerStack::iterator LayerStack::begin()
	{
		return m_Layers.begin();
	}

	LayerStack::const_iterator LayerStack::cbegin() const
	{
		return m_Layers.cbegin();
	}

	LayerStack::iterator LayerStack::end()
	{
		return m_Layers.end();
	}

	LayerStack::const_iterator LayerStack::cend() const
	{
		return m_Layers.cend();
	}

	LayerStack::reverse_iterator LayerStack::rbegin()
	{
		return m_Layers.rbegin();
	}

	LayerStack::const_reverse_iterator LayerStack::crbegin() const
	{
		return m_Layers.crbegin();
	}

	LayerStack::reverse_iterator LayerStack::rend()
	{
		return m_Layers.rend();
	}

	LayerStack::const_reverse_iterator LayerStack::crend() const
	{
		return m_Layers.crend();
	}
}
