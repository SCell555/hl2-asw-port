//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CONTROLGRID_H
#define CONTROLGRID_H

#ifdef _WIN32
#pragma once
#endif

#include <UtlLinkedList.h>
#include <UtlVector.h>
#include <vgui/VGUI.h>
#include <vgui_controls/Panel.h>

class KeyValues;

namespace vgui
{

	//-----------------------------------------------------------------------------
	// Purpose: A list of variable height child panels
	//  each list item consists of a label-panel pair. Height of the item is
	// determined from the lable.
	//-----------------------------------------------------------------------------
	class ControlGrid : public Panel
	{
		DECLARE_CLASS_SIMPLE(ControlGrid, Panel);

	public:
		ControlGrid(vgui::Panel *parent, char const *panelName);
		~ControlGrid();

		// DATA & ROW HANDLING
		// The list now owns the panel
		virtual int AddItem(Panel *panel, Panel *labelPanel = NULL);
		virtual int	GetItemCount() const;

		virtual Panel *GetItemLabel(int itemID);
		virtual Panel *GetItemPanel(int itemID);

		virtual void RemoveItem(int itemID); // removes an item from the table (changing the indices of all following items)
		virtual void DeleteAllItems(); // clears and deletes all the memory used by the data items
		void RemoveAll();

		// painting
		virtual vgui::Panel *GetCellRenderer(int row);

		// layout
		void MoveScrollBarToTop();

		// selection
		void SetSelectedPanel(Panel *panel);
		Panel *GetSelectedPanel();
		/*
		On a panel being selected, a message gets sent to it
		"PanelSelected"		int "state"
		where state is 1 on selection, 0 on deselection
		*/

		void		SetVerticalBufferPixels(int buffer);

		//added functionality
		int GetItemIDFromRow(int nRow) const;
		int FirstItem() const;
		int NextItem(int nItemID) const;
		int InvalidItemID() const;
		void SetNumColumns(int iNumColumns);
		int GetNumColumns(void);
		void ScrollToItem(int itemNumber);
		//void CalcBestWidth( int LargestControlWidth = 0 );
		void SetShouldDrawLabels(bool b) { m_bShouldDrawLabels = b; }
		bool GetShouldDrawLabels() { return m_bShouldDrawLabels; }
		void SetLabelHeight(int i) { m_iLabelHeight = i; }
		int  GetLabelHeight() { return m_iLabelHeight; }
		//default is 20

	protected:
		// overrides
		virtual void OnSizeChanged(int wide, int tall);
		MESSAGE_FUNC_INT(OnSliderMoved, "ScrollBarSliderMoved", position);
		virtual void PerformLayout();
		virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
		virtual void OnMouseWheeled(int delta);

	private:
		int	ComputeVPixelsNeeded();

		enum { DEFAULT_HEIGHT = 24, PANELBUFFER = 5 };

		typedef struct dataitem_s
		{
			// Always store a panel pointer
			Panel *panel;
			Panel *labelPanel;
		} DATAITEM;

		// list of the column headers

		CUtlLinkedList<DATAITEM, int>		m_DataItems;
		CUtlVector<int>						m_SortedItems;

		ScrollBar				*m_vbar;
		Panel					*m_pPanelEmbedded;

		PHandle					m_hSelectedItem;
		int						m_iDefaultHeight;
		int						m_iPanelBuffer;
		int						m_iNumColumns;
		int						m_iLabelHeight;
		bool					m_bShouldDrawLabels;
	};

}
#endif // CONTROLGRID_H