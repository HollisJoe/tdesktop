/*
This file is part of Telegram Desktop,
the official desktop version of Telegram messaging app, see https://telegram.org

Telegram Desktop is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

It is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

In addition, as a special exception, the copyright holders give permission
to link the code of portions of this program with the OpenSSL library.

Full license: https://github.com/telegramdesktop/tdesktop/blob/master/LICENSE
Copyright (c) 2014-2017 John Preston, https://desktop.telegram.org
*/
#pragma once

#include "ui/rp_widget.h"
#include "info/media/info_media_widget.h"
#include "data/data_shared_media.h"

namespace Ui {
class PopupMenu;
} // namespace Ui

namespace Overview {
namespace Layout {
class ItemBase;
} // namespace Layout
} // namespace Overview

namespace Window {
class Controller;
} // namespace Window

namespace Info {

class AbstractController;

namespace Media {

using BaseLayout = Overview::Layout::ItemBase;
using UniversalMsgId = int32;

class ListWidget : public Ui::RpWidget {
public:
	ListWidget(
		QWidget *parent,
		not_null<AbstractController*> controller);

	void restart();

	rpl::producer<int> scrollToRequests() const;
	rpl::producer<SelectedItems> selectedListValue() const;
	void cancelSelection() {
		clearSelected();
	}

	QRect getCurrentSongGeometry();
	rpl::producer<> checkForHide() const {
		return _checkForHide.events();
	}
	bool preventAutoHide() const {
		return (_contextMenu != nullptr) || (_actionBoxWeak != nullptr);
	}

	void saveState(not_null<Memento*> memento);
	void restoreState(not_null<Memento*> memento);

	~ListWidget();

protected:
	int resizeGetHeight(int newWidth) override;
	void visibleTopBottomUpdated(
		int visibleTop,
		int visibleBottom) override;

	void paintEvent(QPaintEvent *e) override;
	void mouseMoveEvent(QMouseEvent *e) override;
	void mousePressEvent(QMouseEvent *e) override;
	void mouseReleaseEvent(QMouseEvent *e) override;
	void mouseDoubleClickEvent(QMouseEvent *e) override;
	void contextMenuEvent(QContextMenuEvent *e) override;
	void enterEventHook(QEvent *e) override;
	void leaveEventHook(QEvent *e) override;

private:
	enum class MouseAction {
		None,
		PrepareDrag,
		Dragging,
		PrepareSelect,
		Selecting,
	};
	struct CachedItem {
		CachedItem(std::unique_ptr<BaseLayout> item);
		~CachedItem();

		std::unique_ptr<BaseLayout> item;
		bool stale = false;
	};
	struct Context;
	class Section;
	struct FoundItem {
		not_null<BaseLayout*> layout;
		QRect geometry;
		bool exact = false;
	};
	struct SelectionData {
		explicit SelectionData(TextSelection text) : text(text) {
		}

		TextSelection text;
		bool canDelete = false;
		bool canForward = false;
	};
	using SelectedMap = base::flat_map<
		UniversalMsgId,
		SelectionData,
		std::less<>>;
	enum class DragSelectAction {
		None,
		Selecting,
		Deselecting,
	};
	struct CursorState {
		UniversalMsgId itemId = 0;
		QSize size;
		QPoint cursor;
		bool inside = false;

		inline bool operator==(const CursorState &other) const {
			return (itemId == other.itemId)
				&& (cursor == other.cursor);
		}
		inline bool operator!=(const CursorState &other) const {
			return !(*this == other);
		}

	};
	enum class ContextMenuSource {
		Mouse,
		Touch,
		Other,
	};
	struct ScrollTopState {
		UniversalMsgId item = 0;
		int shift = 0;
	};

	void start();
	int recountHeight();
	void refreshHeight();

	QMargins padding() const;
	bool isMyItem(not_null<const HistoryItem*> item) const;
	bool isItemLayout(
		not_null<const HistoryItem*> item,
		BaseLayout *layout) const;
	bool isPossiblyMyId(FullMsgId fullId) const;
	void repaintItem(const HistoryItem *item);
	void repaintItem(UniversalMsgId msgId);
	void repaintItem(const BaseLayout *item);
	void repaintItem(QRect itemGeometry);
	void itemRemoved(not_null<const HistoryItem*> item);
	void itemLayoutChanged(not_null<const HistoryItem*> item);

	void refreshViewer();
	void invalidatePaletteCache();
	void refreshRows();
	SparseIdsMergedSlice::Key sliceKey(
		UniversalMsgId universalId) const;
	BaseLayout *getLayout(UniversalMsgId universalId);
	BaseLayout *getExistingLayout(UniversalMsgId universalId) const;
	std::unique_ptr<BaseLayout> createLayout(
		UniversalMsgId universalId,
		Type type);

	SelectedItems collectSelectedItems() const;
	MessageIdsList collectSelectedIds() const;
	void pushSelectedItems();
	FullMsgId computeFullId(UniversalMsgId universalId) const;
	bool hasSelected() const;
	bool isSelectedItem(
		const SelectedMap::const_iterator &i) const;
	void removeItemSelection(
		const SelectedMap::const_iterator &i);
	bool hasSelectedText() const;
	bool hasSelectedItems() const;
	void clearSelected();
	void forwardSelected();
	void forwardItem(UniversalMsgId universalId);
	void forwardItems(MessageIdsList &&items);
	void deleteSelected();
	void deleteItem(UniversalMsgId universalId);
	void deleteItems(MessageIdsList &&items);
	void applyItemSelection(
		UniversalMsgId universalId,
		TextSelection selection);
	void toggleItemSelection(
		UniversalMsgId universalId);
	SelectedMap::iterator itemUnderPressSelection();
	SelectedMap::const_iterator itemUnderPressSelection() const;
	bool isItemUnderPressSelected() const;
	bool requiredToStartDragging(not_null<BaseLayout*> layout) const;
	bool isPressInSelectedText(HistoryTextState state) const;
	void applyDragSelection();
	void applyDragSelection(SelectedMap &applyTo) const;
	bool changeItemSelection(
		SelectedMap &selected,
		UniversalMsgId universalId,
		TextSelection selection) const;

	static bool IsAfter(
		const CursorState &a,
		const CursorState &b);
	static bool SkipSelectFromItem(const CursorState &state);
	static bool SkipSelectTillItem(const CursorState &state);

	void markLayoutsStale();
	void clearStaleLayouts();
	std::vector<Section>::iterator findSectionByItem(
		UniversalMsgId universalId);
	std::vector<Section>::iterator findSectionAfterTop(int top);
	std::vector<Section>::const_iterator findSectionAfterTop(
		int top) const;
	std::vector<Section>::const_iterator findSectionAfterBottom(
		std::vector<Section>::const_iterator from,
		int bottom) const;
	FoundItem findItemByPoint(QPoint point) const;
	base::optional<FoundItem> findItemById(UniversalMsgId universalId);
	base::optional<FoundItem> findItemDetails(BaseLayout *item);
	FoundItem foundItemInSection(
		const FoundItem &item,
		const Section &section) const;

	ScrollTopState countScrollState() const;
	void saveScrollState();
	void restoreScrollState();

	QPoint clampMousePosition(QPoint position) const;
	void mouseActionStart(
		const QPoint &screenPos,
		Qt::MouseButton button);
	void mouseActionUpdate(const QPoint &screenPos);
	void mouseActionUpdate();
	void mouseActionFinish(
		const QPoint &screenPos,
		Qt::MouseButton button);
	void mouseActionCancel();
	void performDrag();
	style::cursor computeMouseCursor() const;
	void showContextMenu(
		QContextMenuEvent *e,
		ContextMenuSource source);

	void updateDragSelection();
	void clearDragSelection();

	void trySwitchToWordSelection();
	void switchToWordSelection();
	void validateTrippleClickStartTime();
	void checkMoveToOtherViewer();

	void setActionBoxWeak(QPointer<Ui::RpWidget> box);

	const not_null<AbstractController*> _controller;
	const not_null<PeerData*> _peer;
	PeerData * const _migrated = nullptr;
	Type _type = Type::Photo;

	static constexpr auto kMinimalIdsLimit = 16;
	static constexpr auto kDefaultAroundId = (ServerMaxMsgId - 1);
	UniversalMsgId _universalAroundId = kDefaultAroundId;
	int _idsLimit = kMinimalIdsLimit;
	SparseIdsMergedSlice _slice;

	std::map<UniversalMsgId, CachedItem> _layouts;
	std::vector<Section> _sections;

	int _visibleTop = 0;
	int _visibleBottom = 0;
	ScrollTopState _scrollTopState;
	rpl::event_stream<int> _scrollToRequests;

	MouseAction _mouseAction = MouseAction::None;
	TextSelectType _mouseSelectType = TextSelectType::Letters;
	QPoint _mousePosition;
	CursorState _overState;
	CursorState _pressState;
	BaseLayout *_overLayout = nullptr;
	UniversalMsgId _contextUniversalId = 0;
	HistoryCursorState _mouseCursorState = HistoryDefaultCursorState;
	uint16 _mouseTextSymbol = 0;
	bool _pressWasInactive = false;
	SelectedMap _selected;
	SelectedMap _dragSelected;
	rpl::event_stream<SelectedItems> _selectedListStream;
	style::cursor _cursor = style::cur_default;
	DragSelectAction _dragSelectAction = DragSelectAction::None;
	bool _wasSelectedText = false; // was some text selected in current drag action

	Ui::PopupMenu *_contextMenu = nullptr;
	rpl::event_stream<> _checkForHide;
	QPointer<Ui::RpWidget> _actionBoxWeak;
	rpl::lifetime _actionBoxWeakLifetime;

	QPoint _trippleClickPoint;
	TimeMs _trippleClickStartTime = 0;

	rpl::lifetime _viewerLifetime;

};

} // namespace Media
} // namespace Info
