#include "cbase.h"
#include "vgui/MouseCode.h"
#include "vgui/IInput.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
 
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/ScrollBar.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/Controls.h"
#include "vgui_controlgrid.h"
 
#include "KeyValues.h"
 
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
 
using namespace vgui;
 
//Adds to "VGUI Build Mode Editor"
DECLARE_BUILD_FACTORY( ControlGrid );
 
//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
ControlGrid::ControlGrid( vgui::Panel *parent, char const *panelName ) : Panel( parent, panelName )
{
	SetBounds( 0, 0, 100, 100 );
 
	m_vbar = new ScrollBar(this, "PanelListPanelVScroll", true);
	m_vbar->SetVisible(false);
	m_vbar->AddActionSignalTarget( this );
 
	m_pPanelEmbedded = new EditablePanel(this, "PanelListEmbedded");
	m_pPanelEmbedded->SetBounds(0, 0, 20, 20);
	m_pPanelEmbedded->SetPaintBackgroundEnabled( false );
	m_pPanelEmbedded->SetPaintBorderEnabled(false);
 
	m_bShouldDrawLabels = false;
	m_iLabelHeight = 20; //default room to leave for labels
 
	//m_iFirstColumnWidth = 100; // default width
	m_iNumColumns = 1; // 1 column by default
 
	if ( IsProportional() )
	{
		m_iDefaultHeight = scheme()->GetProportionalScaledValueEx( GetScheme(), DEFAULT_HEIGHT );
		m_iPanelBuffer = scheme()->GetProportionalScaledValueEx( GetScheme(), PANELBUFFER );
	}
	else
	{
		m_iDefaultHeight = DEFAULT_HEIGHT;
		m_iPanelBuffer = PANELBUFFER;
	}
}
 
//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
ControlGrid::~ControlGrid()
{
	// free data from table
	DeleteAllItems();
}
 
void ControlGrid::SetVerticalBufferPixels( int buffer )
{
	m_iPanelBuffer = buffer;
	InvalidateLayout();
}
 
//-----------------------------------------------------------------------------
// Purpose: counts the total vertical pixels
//-----------------------------------------------------------------------------
int	ControlGrid::ComputeVPixelsNeeded()
{
	int iLargestH = 0;
	int w, h;
 
	int pixels = 0;
	for ( int iCurrentItem = 0; iCurrentItem < m_SortedItems.Count(); iCurrentItem++ )
	{
		Panel *panel = m_DataItems[ m_SortedItems[iCurrentItem] ].panel;
		if ( !panel )
			continue;
 
		int iCurrentColumn = iCurrentItem % m_iNumColumns;
 
		panel->GetSize( w, h );
 
		if ( iLargestH < h )
			iLargestH = h;
 
		if ( iCurrentColumn == 0 && (!m_bShouldDrawLabels || iCurrentItem == 0) )
			pixels += m_iPanelBuffer; // add in buffer. between rows.
 
		if ( iCurrentColumn == m_iNumColumns - 1 && iCurrentItem != m_SortedItems.Count()-1 )
		{
			if (m_bShouldDrawLabels)
				pixels += m_iLabelHeight;
 
			pixels += iLargestH;
			iLargestH = 0;
		}
	}
 
	// Add in remaining height
	if (m_bShouldDrawLabels)
		pixels += m_iLabelHeight;
 
	pixels += iLargestH;
 
	pixels += m_iPanelBuffer; // add in buffer below last item
 
	return pixels;
}
 
//-----------------------------------------------------------------------------
// Purpose: Returns the panel to use to render a cell
//-----------------------------------------------------------------------------
Panel *ControlGrid::GetCellRenderer( int row )
{
	if ( !m_SortedItems.IsValidIndex(row) )
		return NULL;
 
	Panel *panel = m_DataItems[ m_SortedItems[row] ].panel;
	return panel;
}
 
//-----------------------------------------------------------------------------
// Purpose: adds an item to the view
//			data->GetName() is used to uniquely identify an item
//			data sub items are matched against column header name to be used in the table
//-----------------------------------------------------------------------------
int ControlGrid::AddItem( Panel *panel, Panel *labelPanel)
{
	Assert(panel);
 
	if ( labelPanel )
	{
		labelPanel->SetParent( m_pPanelEmbedded );
	}
 
	panel->SetParent( m_pPanelEmbedded );
 
	int itemID = m_DataItems.AddToTail();
	DATAITEM &newitem = m_DataItems[itemID];
	newitem.labelPanel = labelPanel;
	newitem.panel = panel;
	m_SortedItems.AddToTail(itemID);
 
	InvalidateLayout();
	return itemID;
}
 
//-----------------------------------------------------------------------------
// Purpose: iteration accessor
//-----------------------------------------------------------------------------
int	ControlGrid::GetItemCount() const
{
	return m_DataItems.Count();
}
 
int ControlGrid::GetItemIDFromRow( int nRow ) const
{
	if ( nRow < 0 || nRow >= GetItemCount() )
		return m_DataItems.InvalidIndex();
	return m_SortedItems[ nRow ];
}
 
 
//-----------------------------------------------------------------------------
// Iteration. Use these until they return InvalidItemID to iterate all the items.
//-----------------------------------------------------------------------------
int ControlGrid::FirstItem() const
{
	return m_DataItems.Head();
}
 
int ControlGrid::NextItem( int nItemID ) const
{
	return m_DataItems.Next( nItemID );
}
 
int ControlGrid::InvalidItemID() const
{
	return m_DataItems.InvalidIndex( );
}
 
 
//-----------------------------------------------------------------------------
// Purpose: returns label panel for this itemID
//-----------------------------------------------------------------------------
Panel *ControlGrid::GetItemLabel(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return NULL;
 
	return m_DataItems[itemID].labelPanel;
}
 
//-----------------------------------------------------------------------------
// Purpose: returns label panel for this itemID
//-----------------------------------------------------------------------------
Panel *ControlGrid::GetItemPanel(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return NULL;
 
	return m_DataItems[itemID].panel;
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ControlGrid::RemoveItem(int itemID)
{
	if ( !m_DataItems.IsValidIndex(itemID) )
		return;
 
	DATAITEM &item = m_DataItems[itemID];
	if ( item.panel )
	{
		item.panel->MarkForDeletion();
	}
	if ( item.labelPanel )
	{
		item.labelPanel->MarkForDeletion();
	}
 
	m_DataItems.Remove(itemID);
	m_SortedItems.FindAndRemove(itemID);
 
	InvalidateLayout();
}
 
//-----------------------------------------------------------------------------
// Purpose: clears and deletes all the memory used by the data items
//-----------------------------------------------------------------------------
void ControlGrid::DeleteAllItems()
{
	FOR_EACH_LL( m_DataItems, i )
	{
		if ( m_DataItems[i].panel )
		{
			delete m_DataItems[i].panel;
		}
	}
 
	m_DataItems.RemoveAll();
	m_SortedItems.RemoveAll();
 
	InvalidateLayout();
}
 
//-----------------------------------------------------------------------------
// Purpose: clears and deletes all the memory used by the data items
//-----------------------------------------------------------------------------
void ControlGrid::RemoveAll()
{
	m_DataItems.RemoveAll();
	m_SortedItems.RemoveAll();
 
	// move the scrollbar to the top of the list
	m_vbar->SetValue(0);
	InvalidateLayout();
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ControlGrid::OnSizeChanged(int wide, int tall)
{
	BaseClass::OnSizeChanged(wide, tall);
	InvalidateLayout();
}
 
//-----------------------------------------------------------------------------
// Purpose: relayouts out the panel after any internal changes
//-----------------------------------------------------------------------------
void ControlGrid::PerformLayout()
{
	int wide, tall;
	GetSize( wide, tall );
 
	int vpixels = ComputeVPixelsNeeded();
 
	m_vbar->SetVisible( true );
	m_vbar->SetRange( 0, vpixels );
	m_vbar->SetRangeWindow( tall );
	m_vbar->SetButtonPressedScrollValue( tall / 4 ); // standard height of labels/buttons etc.
 
	m_vbar->SetPos( wide - m_vbar->GetWide() - 2, 0 );
	m_vbar->SetSize( m_vbar->GetWide(), tall - 2 );
 
	int top = m_vbar->GetValue();
 
	m_pPanelEmbedded->SetPos( 1, -top );
	m_pPanelEmbedded->SetSize( wide - m_vbar->GetWide() - 2, vpixels );
 
	int sliderPos = m_vbar->GetValue();
 
	// Now lay out the controls on the embedded panel
	int y = 0;
	int h = 0;
	int totalh = 0;
 
	int xpos = m_iPanelBuffer;
	int iColumnWidth = ( wide - xpos - m_vbar->GetWide() - 12 ) / m_iNumColumns;
 
	for ( int i = 0; i < m_SortedItems.Count(); i++ )
	{
		int iCurrentColumn = i % m_iNumColumns;
 
		// add in a little buffer between panels
		if ( iCurrentColumn == 0 && (!m_bShouldDrawLabels || i == 0) )
		{
			y += m_iPanelBuffer;
		}
 
		DATAITEM &item = m_DataItems[ m_SortedItems[i] ];
 
		if ( h < item.panel->GetTall() )
			h = item.panel->GetTall();
 
		if (totalh >= sliderPos)
		{
			if ( item.labelPanel && m_bShouldDrawLabels )
			{
				item.labelPanel->SetBounds( xpos + iCurrentColumn*iColumnWidth, y + item.panel->GetTall(), iColumnWidth, m_iLabelHeight );
			}
 
			item.panel->SetBounds( xpos + iCurrentColumn * iColumnWidth, y, iColumnWidth, item.panel->GetTall() );
		}
 
		if ( iCurrentColumn >= m_iNumColumns - 1 )
		{
			if (m_bShouldDrawLabels)
				h += m_iLabelHeight;
 
			y += h;
			totalh += h;
 
			h = 0;
		}
	}
}
 
 
//------------------------------------------------------------------------------------------------------
// Purpose: determines the best width for the control based on the width of inserted elements
// Note: This function is commented out because control elements do not always give accurate information
//       from their control->GetWidth() functions. (who knew...) Otherwise, it works fine.
//------------------------------------------------------------------------------------------------------
/*
void ControlGrid::CalcBestWidth(int LargestControlWidth)
{
	int width = m_iPanelBuffer;
	CUtlVector<int> rowWidths;
	int w, h;
 
	//create vector containing widths of rows
	for ( int i = 0; i < m_SortedItems.Count(); i++ )
	{
		int iCurrentColumn = i % m_iNumColumns;
 
		DATAITEM &item = m_DataItems[ m_SortedItems[i] ];
 
		int trueWidth = item.panel->GetWide();
		if (trueWidth < LargestControlWidth)
			trueWidth = LargestControlWidth;
 
		width += trueWidth + m_iPanelBuffer;
 
		if ( iCurrentColumn >= m_iNumColumns - 1 )
		{
			rowWidths.AddToTail(width);
			width = 0;
		}
	}
 
	//compare widths of rows, see which is largest and use it
	for ( int i = 0; i < rowWidths.Count(); i++ )
	{
		bool isLargest = true;
 
		for ( int j = 0; j < rowWidths.Count(); j++ )
		{
			if ( rowWidths.Element(i) < rowWidths.Element(j) )
			{
				isLargest = false;
				break;
			}
		}
 
		if (isLargest)
		{
			width = rowWidths.Element(i);
			break;
		}
	}
 
	SetSize( width ? width : 100, GetTall() );
}
*/
//-----------------------------------------------------------------------------
// Purpose: scheme settings
//-----------------------------------------------------------------------------
void ControlGrid::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
 
	SetBorder(pScheme->GetBorder("ButtonDepressedBorder"));
	SetBgColor(GetSchemeColor("ListPanel.BgColor", GetBgColor(), pScheme));
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ControlGrid::OnSliderMoved( int position )
{
	InvalidateLayout();
	Repaint();
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ControlGrid::MoveScrollBarToTop()
{
	m_vbar->SetValue(0);
}
 
void ControlGrid::SetNumColumns( int iNumColumns )
{
	m_iNumColumns = iNumColumns;
}
 
int ControlGrid::GetNumColumns( void )
{
	return m_iNumColumns;
}
 
//-----------------------------------------------------------------------------
// Purpose: moves the scrollbar with the mousewheel
//-----------------------------------------------------------------------------
void ControlGrid::OnMouseWheeled(int delta)
{
	int val = m_vbar->GetValue();
	val -= (delta * DEFAULT_HEIGHT);
	m_vbar->SetValue(val);	
}
 
//-----------------------------------------------------------------------------
// Purpose: selection handler
//-----------------------------------------------------------------------------
void ControlGrid::SetSelectedPanel( Panel *panel )
{
	if ( panel != m_hSelectedItem )
	{
		// notify the panels of the selection change
		if ( m_hSelectedItem )
		{
			PostMessage( m_hSelectedItem, new KeyValues("PanelSelected", "state", 0) );
		}
		if ( panel )
		{
			PostMessage( panel, new KeyValues("PanelSelected", "state", 1) );
		}
		m_hSelectedItem = panel;
	}
}
 
//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
Panel *ControlGrid::GetSelectedPanel()
{
	return m_hSelectedItem;
}
 
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ControlGrid::ScrollToItem( int itemNumber )
{
	if (!m_vbar->IsVisible())
	{
		return;
	}
 
	DATAITEM& item = m_DataItems[ m_SortedItems[ itemNumber ] ];
	if ( !item.panel )
		return;
 
	int x, y;
	item.panel->GetPos( x, y );
	int lx, ly;
	lx = x;
	ly = y;
	m_pPanelEmbedded->LocalToScreen( lx, ly );
	ScreenToLocal( lx, ly );
 
	int h = item.panel->GetTall();
 
	if ( ly >= 0 && ly + h < GetTall() )
		return;
 
	m_vbar->SetValue( y );
	InvalidateLayout();
}