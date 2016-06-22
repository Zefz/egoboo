//********************************************************************************************
//*
//*    This file is part of Egoboo.
//*
//*    Egoboo is free software: you can redistribute it and/or modify it
//*    under the terms of the GNU General Public License as published by
//*    the Free Software Foundation, either version 3 of the License, or
//*    (at your option) any later version.
//*
//*    Egoboo is distributed in the hope that it will be useful, but
//*    WITHOUT ANY WARRANTY; without even the implied warranty of
//*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//*    General Public License for more details.
//*
//*    You should have received a copy of the GNU General Public License
//*    along with Egoboo.  If not, see <http://www.gnu.org/licenses/>.
//*
//********************************************************************************************

/// @file game/GUI/ScrollableList.hpp
/// @details GUI widget that can contain any number of GUIComponents and that can scroll through
///			 each component contained within this list
/// @author Johan Jansen

#pragma once

#include "game/GUI/ComponentContainer.hpp"
#include "game/GUI/GUIComponent.hpp"

//Forward declaration
class Button;

class ScrollableList : public GUIComponent, public ComponentContainer
{
public:
	static const size_t COMPONENT_LINE_SPACING;

	ScrollableList();

	void draw() override;
    
    /**
     * @brief
     *  Force this to update the scroll bars and child components.
     * @note
     *  This should be unnecessary when children notify their parent
     *  on size change and when they are added or removed.
     */
    void forceUpdate();

    /**
    * Ensure that this class inherits defaults for these methods from ComponentContainer and not GUIComponent
    **/
    bool notifyKeyDown(const int keyCode) override {
        return ComponentContainer::notifyKeyDown(keyCode);
    }
	bool notifyMouseClicked(const Ego::Events::MouseClickedEventArgs& e) override;
	bool notifyMouseScrolled(const int amount) override;
	bool notifyMouseMoved(const Ego::Events::MouseMovedEventArgs& e) override;

    void setWidth(float width) override;
    void setHeight(float height) override;
    void setX(float x) override;
    void setY(float y) override;
    void setPosition(float x, float y) override;

protected:
	void drawContainer() override;
	void updateScrollButtons();
	void setScrollPosition(int position);

private:
	size_t _currentIndex;
	bool _mouseOver;
	std::shared_ptr<Button> _downButton;
	std::shared_ptr<Button> _upButton;
};
