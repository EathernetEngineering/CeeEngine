/* Implementation of debug layer.
 *   Copyright (C) 2023-2024  Chloe Eather.
 *
 *   This file is part of CeeEngine.
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>. */

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
