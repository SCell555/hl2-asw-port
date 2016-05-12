#ifndef INVENTORYWINDOW_H
#define INVENTORYWINDOW_H

class IInventoryPanel
{
public:
	virtual void Create(vgui::VPANEL parent) = 0; // The function to create our gui.
	virtual void Destroy(void) = 0; // The function to destory our gui.
};

extern IInventoryPanel* InventoryPanel;

#endif