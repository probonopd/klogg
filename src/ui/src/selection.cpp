/*
 * Copyright (C) 2010, 2013 Nicolas Bonnefon and other contributors
 *
 * This file is part of glogg.
 *
 * glogg is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * glogg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with glogg.  If not, see <http://www.gnu.org/licenses/>.
 */

// This file implements Selection.
// This class implements the selection handling. No check is made on
// the validity of the selection, it must be handled by the caller.
// There are three types of selection, only one type might be active
// at any time.

#include "selection.h"

#include "data/abstractlogdata.h"

Selection::Selection()
{
    selectedPartial_.startColumn = 0;
    selectedPartial_.endColumn   = 0;

    selectedRange_.endLine   = 0_lnum;
}

void Selection::selectPortion( LineNumber line, int start_column, int end_column )
{
    // First unselect any whole line or range
    selectedLine_ = {};
    selectedRange_.startLine = {};

    selectedPartial_.line = line;
    selectedPartial_.startColumn = qMin ( start_column, end_column );
    selectedPartial_.endColumn   = qMax ( start_column, end_column );
}

void Selection::selectRange( LineNumber start_line, LineNumber end_line )
{
    // First unselect any whole line and portion
    selectedLine_ = {};
    selectedPartial_.line = {};

    selectedRange_.startLine = qMin ( start_line, end_line );
    selectedRange_.endLine   = qMax ( start_line, end_line );

    selectedRange_.firstLine = start_line;
}

void Selection::selectRangeFromPrevious( LineNumber line )
{
    LineNumber previous_line;

    if ( selectedLine_.has_value() )
        previous_line = *selectedLine_;
    else if ( selectedRange_.startLine.has_value() )
        previous_line = selectedRange_.firstLine;
    else if ( selectedPartial_.line.has_value() )
        previous_line = *selectedPartial_.line;
    else
        previous_line = 0_lnum;

    selectRange( previous_line, line );
}

void Selection::crop( LineNumber last_line )
{
    if ( selectedLine_.has_value() &&  *selectedLine_ > last_line )
        selectedLine_ = {};

    if ( selectedPartial_.line.has_value() && *selectedPartial_.line > last_line )
        selectedPartial_.line = {};

    if ( selectedRange_.endLine > last_line )
        selectedRange_.endLine = last_line;

    if ( selectedRange_.startLine.has_value() && *selectedRange_.startLine > last_line )
        selectedRange_.startLine = last_line;
}

bool Selection::getPortionForLine( LineNumber line, int* start_column, int* end_column ) const
{
    if ( selectedPartial_.line.has_value() && *selectedPartial_.line == line ) {
        *start_column = selectedPartial_.startColumn;
        *end_column   = selectedPartial_.endColumn;

        return true;
    }
    else {
        return false;
    }
}

bool Selection::isLineSelected( LineNumber line ) const
{
    if ( selectedLine_.has_value()  && line == *selectedLine_ )
        return true;
    else if ( selectedRange_.startLine.has_value() )
        return ( ( line >= *selectedRange_.startLine )
                && ( line <= selectedRange_.endLine ) );
    else
        return false;
}

OptionalLineNumber Selection::selectedLine() const
{
    return selectedLine_;
}

std::vector<LineNumber> Selection::getLines() const
{
    std::vector<LineNumber> selection;

    if ( selectedLine_.has_value() ) {
        selection.push_back( *selectedLine_ );
    }
    else if ( selectedPartial_.line.has_value() ) {
        selection.push_back( *selectedPartial_.line );
    }
    else if ( selectedRange_.startLine.has_value() ) {
        selection.reserve( selectedRange_.endLine.get() - selectedRange_.startLine->get() + 1 );
        for ( auto i = *selectedRange_.startLine; i <= selectedRange_.endLine; ++i )
            selection.push_back( i );
    }

    return selection;
}

// The tab behaviour is a bit odd at the moment, full lines are not expanded
// but partials (part of line) are, they probably should not ideally.
QString Selection::getSelectedText( const AbstractLogData* logData ) const
{
    QString text;

    if ( selectedLine_.has_value() ) {
        text = logData->getLineString( *selectedLine_ );
    }
    else if ( selectedPartial_.line.has_value() ) {
        text = logData->getExpandedLineString( *selectedPartial_.line ).
            mid( selectedPartial_.startColumn, ( selectedPartial_.endColumn -
                        selectedPartial_.startColumn ) + 1 );
    }
    else if ( selectedRange_.startLine.has_value() ) {
        QStringList list = logData->getLines( *selectedRange_.startLine,
                LinesCount( selectedRange_.endLine.get() - selectedRange_.startLine->get() + 1 ) );
        text = list.join( QChar::LineFeed );
    }

    return text;
}

FilePosition Selection::getNextPosition() const
{
    LineNumber line;
    int column = 0;

    if ( selectedLine_.has_value() ) {
        line = *selectedLine_ + 1_lcount;
    }
    else if ( selectedRange_.startLine.has_value() ) {
        line = selectedRange_.endLine + 1_lcount;
    }
    else if ( selectedPartial_.line.has_value() ) {
        line   = *selectedPartial_.line;
        column = selectedPartial_.endColumn + 1;
    }

    return FilePosition( line, column );
}

FilePosition Selection::getPreviousPosition() const
{
    LineNumber line;
    int column = 0;

    if ( selectedLine_.has_value() ) {
        line = *selectedLine_;
    }
    else if ( selectedRange_.startLine.has_value() ) {
        line = *selectedRange_.startLine;
    }
    else if ( selectedPartial_.line.has_value() ) {
        line   = *selectedPartial_.line;
        column = qMax( selectedPartial_.startColumn - 1, 0 );
    }

    return FilePosition( line, column );
}
