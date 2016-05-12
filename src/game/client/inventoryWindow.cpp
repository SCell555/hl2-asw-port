#include "cbase.h"
#include "inventoryWindow.h"
#include "c_basehlplayer.h"

using namespace vgui;

#include "vgui/IVGui.h"
#include "vgui_controls\Frame.h"
#include "vgui_controls/ListPanel.h"
#include "vgui_controls\Button.h"
#include "vgui_controls/ProgressBar.h"
#include "vgui_controls/Label.h"
#include "advmodelpanel.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


class CInventoryWindow : public Frame
{
	DECLARE_CLASS_SIMPLE(CInventoryWindow, Frame);
public:
	CInventoryWindow(vgui::VPANEL parent);
	~CInventoryWindow(){};

	void ApplySchemeSettings(IScheme *pScheme);
	void OnCommand(const char* command);
protected:
	virtual void OnTick();
	virtual void OnClose();
private:
	ListPanel* table;
	ContinuousProgressBar *fill;
	Button*		drop;
	Button*		close;
	CAdvModelPanel* model;
	int lastSelected;
	int lastCount;
	char*	mdlName;
};

int __cdecl SortItems(ListPanel *pPanel, const ListPanelItem &item1, const ListPanelItem &item2)
{
	return item2.kv->GetInt("invnum") - item1.kv->GetInt("invnum");
}

CInventoryWindow::CInventoryWindow(vgui::VPANEL parent) : BaseClass(NULL, "Inventory")
{
	SetParent(parent);
	mdlName = NULL;
	table = SETUP_PANEL(new ListPanel(this, "table"));
	fill = SETUP_PANEL(new ContinuousProgressBar(this, "space"));
	drop = SETUP_PANEL(new Button(this, "dropButton", "#Drop", this, "dropItem"));
	close = SETUP_PANEL(new Button(this, "closeButton", "#Close", this, "close"));
	model = SETUP_PANEL(new CAdvModelPanel(this, "preview"));
	model->SetVisible(true);
	model->m_bAllowRotation = true;

	lastSelected = -1;
	lastCount = -1;

	int h, w;
	
	GetHudSize(w, h);

	SetPos(10, 10);

	SetSize(int(w - w / 10.f), int(h - h / 10.f));

	table->SetMultiselectEnabled(false);
	table->AddActionSignalTarget(this);
	table->AddColumnHeader(0, "classname", "Class Name", 512, ListPanel::COLUMN_FIXEDSIZE | ListPanel::COLUMN_RESIZEWITHWINDOW | ListPanel::COLUMN_UNHIDABLE);
	table->AddColumnHeader(1, "name", "Name", 512, ListPanel::COLUMN_FIXEDSIZE | ListPanel::COLUMN_RESIZEWITHWINDOW | ListPanel::COLUMN_UNHIDABLE);
	//table->AddColumnHeader(2, "model", "Model", 1,ListPanel::COLUMN_HIDDEN | ListPanel::COLUMN_UNHIDABLE);
	table->AddColumnHeader(2, "invnum", "No.", 32, ListPanel::COLUMN_FIXEDSIZE);
	table->SetSortFunc(2, SortItems);
	
	ivgui()->AddTickSignal(GetVPanel(), 100);
}

void CInventoryWindow::ApplySchemeSettings(IScheme *pScheme)
{
	SetScheme(scheme()->GetDefaultScheme());

	LoadControlSettings("resource/UI/InventoryPanel.res");
	
	BaseClass::ApplySchemeSettings(pScheme);
}

bool g_bNeedUpdate = false;

ConVar inventory("inventory", "0");

void CInventoryWindow::OnTick()
{
	BaseClass::OnTick();

	C_BaseHLPlayer* player = (C_BaseHLPlayer*)C_BaseHLPlayer::GetLocalPlayer();
	
	if (!player)
		return;

	if (inventory.GetBool() != IsVisible())
		SetVisible(inventory.GetBool());

	if (lastSelected != table->GetSelectedItem(0) && lastCount != -1 && table->GetSelectedItem(0) != -1)
	{
		lastSelected = table->GetSelectedItem(0);
		if (mdlName && mdlName[0])
			delete[] mdlName;
		const char* newName = table->GetItemData(lastSelected)->kv->GetString("model");
		mdlName = new char[DT_MAX_STRING_BUFFERSIZE];
		Q_memcpy(mdlName, newName, DT_MAX_STRING_BUFFERSIZE * sizeof(char));
		model->SetModelName(mdlName);
		model->Update();
	}

	
	if (!g_bNeedUpdate)
		return;

	int k = -1;

	for (int i = 0; i < 32; i++)
		if (player->m_Inventory.isOccupied.Get(i))
			k++;

	if (k == lastCount)
		return;

	lastCount = -1;
	for (int i = 0; i < 32; i++)
		if (player->m_Inventory.isOccupied.Get(i))
			lastCount++;

	table->RemoveAll();
	
	g_bNeedUpdate = false;
	auto &inv = player->m_Inventory;
	int nO = 0;
	for (int i = 0; i < 32; i++)
	{
		if (!inv.isOccupied.Get(i))
			continue;

		KeyValues* kv = new KeyValues("Item");

		kv->SetString("classname", inv.classNames[i]);
		kv->SetString("name", (inv.entNames[i] && *inv.entNames[i]) ? inv.entNames[i] : "None");
		kv->SetString("model", inv.modelNames[i]);
		kv->SetInt("invnum", i);

		table->AddItem(kv, NULL, false, false);
		kv->deleteThis();
		nO++;
	}

	table->SetSortColumn(2);
	table->SortList();

	fill->SetProgress((float)nO / (float)32.f);
	if (fill->GetProgress() > 0.8f)
		fill->SetFgColor(COLOR_RED);
	else if (fill->GetProgress() > 0.5f)
		fill->SetFgColor(COLOR_YELLOW);
	else 
		fill->SetFgColor(COLOR_GREEN);
}

void CInventoryWindow::OnClose()
{
	inventory.SetValue(false);
	BaseClass::OnClose();
}

void CInventoryWindow::OnCommand(const char* command)
{
	if (FStrEq(command, "dropItem"))
	{
		engine->ClientCmd_Unrestricted(VarArgs("dropItem %i\n", (table->GetSelectedItem(0) != -1) ? table->GetItemData(table->GetSelectedItem(0))->kv->GetInt("invNum", -1) : -1));
	}
	else if (FStrEq(command, "close"))
	{
		inventory.SetValue(false);
	}
}

class CInventoryPanelInterface : public IInventoryPanel
{
private:
	CInventoryWindow *InventoryPanel;
public:
	CInventoryPanelInterface()
	{
		InventoryPanel = NULL;
	}
	void Create(vgui::VPANEL parent)
	{
		InventoryPanel = new CInventoryWindow(parent);
	}
	void Destroy()
	{
		if (InventoryPanel)
		{
			InventoryPanel->SetParent((vgui::Panel *)NULL);
			delete InventoryPanel;
		}
	}
};

static CInventoryPanelInterface g_InventoryPanel;
IInventoryPanel* InventoryPanel = (IInventoryPanel*)&g_InventoryPanel;