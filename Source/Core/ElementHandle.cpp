/*
 * This source file is part of libRocket, the HTML/CSS Interface Middleware
 *
 * For the latest information, see http://www.librocket.com
 *
 * Copyright (c) 2008-2010 CodePoint Ltd, Shift Technology Ltd
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "precompiled.h"
#include "ElementHandle.h"
#include "../../Include/Rocket/Core/ElementDocument.h"
#include "../../Include/Rocket/Core/ElementUtilities.h"
#include "../../Include/Rocket/Core/Property.h"
#include "../../Include/Rocket/Core/Event.h"

namespace Rocket {
namespace Core {

ElementHandle::ElementHandle(const String& tag) : Element(tag), drag_start(0, 0)
{
	// Make sure we can be dragged!
	SetProperty(PropertyId::Drag, "drag");

	move_target = NULL;
	size_target = NULL;
	initialised = false;
}

ElementHandle::~ElementHandle()
{
}

void ElementHandle::OnAttributeChange(const PropertyNameList& changed_attributes)
{
	Element::OnAttributeChange(changed_attributes);

	// Reset initialised state if the move or size targets have changed.
	if (changed_attributes.find("move_target") != changed_attributes.end() ||
		changed_attributes.find("size_target") != changed_attributes.end())
	{
		initialised = false;
		move_target = NULL;
		size_target = NULL;
	}
}

void ElementHandle::ProcessEvent(Event& event)
{
	Element::ProcessEvent(event);

	if (event.GetTargetElement() == this)
	{
		// Lazy initialisation.
		if (!initialised && GetOwnerDocument())
		{
			String move_target_name = GetAttribute<String>("move_target", "");
			if (!move_target_name.empty())
				move_target = GetElementById(move_target_name);

			String size_target_name = GetAttribute<String>("size_target", "");
			if (!size_target_name.empty())
				size_target = GetElementById(size_target_name);

			initialised = true;
		}

		if (event == EventId::Dragstart)
		{
			// Store the drag starting position
			drag_start.x = event.GetParameter< int >("mouse_x", 0);
			drag_start.y = event.GetParameter< int >("mouse_y", 0);

			// Store current element position and size
			if (move_target)
			{
				move_original_position.x = move_target->GetOffsetLeft();
				move_original_position.y = move_target->GetOffsetTop();
			}
			if (size_target)
				size_original_size = size_target->GetBox().GetSize(Box::CONTENT);
		}
		else if (event == EventId::Drag)
		{
			// Work out the delta
			int x = event.GetParameter< int >("mouse_x", 0) - drag_start.x;
			int y = event.GetParameter< int >("mouse_y", 0) - drag_start.y;

			// Update the move and size objects
			if (move_target)
			{
				move_target->SetProperty(PropertyId::Left, Property(Math::RealToInteger(move_original_position.x + x), Property::PX));
				move_target->SetProperty(PropertyId::Top, Property(Math::RealToInteger(move_original_position.y + y), Property::PX));
			}

			if (size_target)
			{
				const Property *margin_top, *margin_bottom, *margin_left, *margin_right;
				size_target->GetMarginProperties(&margin_top, &margin_bottom, &margin_left, &margin_right);

				// Check if we have auto-margins; if so, they have to be set to the current margins.
				if (margin_top->unit == Property::KEYWORD)
					size_target->SetProperty(PropertyId::MarginTop, Property((float) Math::RealToInteger(size_target->GetBox().GetEdge(Box::MARGIN, Box::TOP)), Property::PX));
				if (margin_right->unit == Property::KEYWORD)
					size_target->SetProperty(PropertyId::MarginRight, Property((float) Math::RealToInteger(size_target->GetBox().GetEdge(Box::MARGIN, Box::RIGHT)), Property::PX));
				if (margin_bottom->unit == Property::KEYWORD)
					size_target->SetProperty(PropertyId::MarginBottom, Property((float) Math::RealToInteger(size_target->GetBox().GetEdge(Box::MARGIN, Box::BOTTOM)), Property::PX));
				if (margin_left->unit == Property::KEYWORD)
					size_target->SetProperty(PropertyId::MarginLeft, Property((float) Math::RealToInteger(size_target->GetBox().GetEdge(Box::MARGIN, Box::LEFT)), Property::PX));

				int new_x = Math::RealToInteger(size_original_size.x + x);
				int new_y = Math::RealToInteger(size_original_size.y + y);

				size_target->SetProperty(PropertyId::Width, Property(Math::Max< float >((float) new_x, 0), Property::PX));
				size_target->SetProperty(PropertyId::Height, Property(Math::Max< float >((float) new_y, 0), Property::PX));
			}

			Dictionary parameters;
			parameters["handle_x"] = x;
			parameters["handle_y"] = y;
			DispatchEvent(EventId::Handledrag, parameters);
		}
	}
}

}
}
