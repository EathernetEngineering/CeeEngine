#include <CeeEngine/debugLayer.h>

#include <CeeEngine/debugMessenger.h>
#include <iostream>

namespace cee {
	DebugLayer::DebugLayer() {

	}

	DebugLayer::~DebugLayer() {

	}

	void DebugLayer::OnAttach() {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_INFO,
										 "Debug layer attached.");
	}

	void DebugLayer::OnDetach() {
		DebugMessenger::PostDebugMessage(ERROR_SEVERITY_INFO,
										 "Debug layer detached.");
	}

	void DebugLayer::OnUpdate(Timestep t) {
		(void)t;
	}

	void DebugLayer::OnRender() {

	}

	void DebugLayer::OnGui() {
	}

	void DebugLayer::OnEnable() {

	}

	void DebugLayer::OnDisable() {

	}

	void DebugLayer::MessageHandler(Event& e) {
		(void)e;
	}


}
