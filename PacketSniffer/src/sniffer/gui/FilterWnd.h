#pragma once

#include "IWindow.h"

namespace sniffer::gui
{
    class FilterWnd :
        public IWindow
    {
    public:
        static FilterWnd& instance();
        void Draw() override;

        WndInfo& GetInfo() override;

    private:
        FilterWnd();

	public:
        FilterWnd(FilterWnd const&) = delete;
		void operator=(FilterWnd const&) = delete;

    };
}
