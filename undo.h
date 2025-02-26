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

#ifndef BDE_UNDO_H
#define BDE_UNDO_H

#include <vector>

#include "types.h"
#include "point.h"

// Class UndoStack records the operations (ops) made to EditBox's buffer.
//
// Strictly speaking, the UndoStack is not really a stack: you move the
// "top" pointer up and down within this stack when you undo and redo
// operations.
// 
// UndoStack stores the operations in a vector of UndoOp's. UndoOp contains
// all the information needed to restore one operation.

enum OpType { opInsert, opDelete, opReplace };

struct UndoOp {

    OpType type;
    Point point; // the point in the buffer where the operation was made
    unistring inserted_text;
    unistring deleted_text;

    size_t calc_size() const {
	return
		sizeof(UndoOp)
		+ inserted_text.len() * sizeof(unichar)
		+ deleted_text.len()  * sizeof(unichar);
    }
    bool merge(const UndoOp &op);
};

class UndoStack {

private:

    std::vector<UndoOp> stack;
    unsigned top;
    
    size_t bytes_size;		// size of current stack

    size_t bytes_size_limit;	// max size

    bool merge_small_ops;	// merge small ops?

    bool truncated;		// was the stack already truncated
				// to fit bytes_size_limit?

private:

    void erase_redo_ops();

    void truncate_undo();
    bool undo_size_too_big() const { return bytes_size >= bytes_size_limit; }
    void update_size_up(int chars_count);
    void update_size_up(const UndoOp &op);
    void update_size_down(const UndoOp &op);

public:

    UndoStack();
    void clear();

    bool was_truncated() { return truncated; }
    bool disabled() const { return bytes_size_limit == 0; }

    bool is_undo_available() const { return (top != 0); }
    bool is_redo_available() const { return (top != stack.size()); }

    void record_op(const UndoOp &op);
    UndoOp *get_prev_op();
    UndoOp *get_next_op();

    void set_size_limit(size_t limit);
    void set_merge(bool value) { merge_small_ops = value; }
    bool is_merge() const { return merge_small_ops; }
};

#endif

