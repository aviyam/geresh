// Copyright (C) 2003 Mooffie <mooffie@typo.co.il>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111, USA.

#include "undo.h"
#include "dbg.h"

#define DEFAULT_LIMIT	4000
#define RESERVE		1000

UndoStack::UndoStack()
{
    top = 0;
    merge_small_ops = false;
    bytes_size = 0;
    bytes_size_limit = DEFAULT_LIMIT;
    truncated = false;
}

void UndoStack::clear()
{
    stack.clear();
    bytes_size = 0;
    top = 0;
    truncated = false;
}

// get_prev_op() - returns a pointer to the last operation made to the buffer.
// returns NULL if the undo stack is empty.

UndoOp* UndoStack::get_prev_op()
{
    if (top == 0)
	return NULL;
    return &stack[--top];
}

// get_next_op() - returns a pointer to the last operation that was undone.

UndoOp *UndoStack::get_next_op()
{
    if (top == stack.size())
	return NULL;
    return &stack[top++];
}

void UndoStack::set_size_limit(size_t limit)
{
    bytes_size_limit = limit;
    clear();
}

void UndoStack::update_size_up(int chars_count)
{
    bytes_size += chars_count * sizeof(unichar);
}

void UndoStack::update_size_up(const UndoOp &op)
{
    bytes_size += op.calc_size();
}

void UndoStack::update_size_down(const UndoOp &op)
{
    bytes_size -= op.calc_size();
}

// truncate_undo() - called when the stack size is too big. it erases the
// old operations.

void UndoStack::truncate_undo()
{
    unsigned new_size = bytes_size_limit;
    // we have to reserve some space in order not to reach the
    // size-limit very soon.
    if (new_size > RESERVE)
	new_size -= RESERVE;
    else
	new_size = 0;
	    
    unsigned i = 0;
    while (i < stack.size() && bytes_size > new_size) {
	update_size_down(stack[i++]);
    }
    stack.erase(stack.begin(), stack.begin() + i);
    top -= i;

    truncated = true;
}

// merge() - merges a new operation with the last operation. returns false if
// that was not possible.
//
// An example: suppose the last operation was to insert "a" into the buffer.
// If the user now types "b", merge() will change the last operation to record
// an insertion of "ab".
//
// However, if the user moves the cursor to a different location before typing
// "b", such a merge is not possible, because "a" and "b" are not adjacent.

bool UndoOp::merge(const UndoOp &op)
{
    if (type != op.type)
	return false;
    if (op.inserted_text.len() + op.deleted_text.len() != 1)
	return false;
    if (point.para != op.point.para)
	return false;
  
    switch (type) {
    case opInsert:
	if (op.point.pos == point.pos + inserted_text.len()) {
	    inserted_text.append(op.inserted_text);
	    return true;
	}
	break;
    case opDelete:
	if (op.point.pos == point.pos) {
	    deleted_text.append(op.deleted_text);
	    return true;
	}
	if (op.point.pos == point.pos - 1) {
	    point.pos--;
	    deleted_text.insert(deleted_text.begin(),
				op.deleted_text.begin(),
				op.deleted_text.end());
	    return true;
	}
	break;
    default:
	break;
    }

    return false;
}

// erase_redo_ops() - erases the operations that were undone.

void UndoStack::erase_redo_ops()
{
    for (unsigned i = top; i < stack.size(); i++)
	update_size_down(stack[i]);
    stack.erase(stack.begin() + top, stack.end());
}

// record_op() - records an operation on the stack.

void UndoStack::record_op(const UndoOp &op)
{
    if (disabled())
	return;

    // recording an opearation erases the recordings of all the operations
    // that were undone till now.
    if (is_redo_available())
	erase_redo_ops();

    if (undo_size_too_big())
	truncate_undo();

    // first try to merge this operation with the previous one. if that
    // fails, record it as a separate operation.
    if (merge_small_ops && !stack.empty() && stack.back().merge(op)) {
	update_size_up(1); // one character was recorded
    } else {
	stack.push_back(op);
	update_size_up(op);
	top++;
    }
}

