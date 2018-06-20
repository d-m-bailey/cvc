/*
 * CFixedText.hh
 *
 * Copyright 2014-2018 D. Mitch Bailey  cvc at shuharisystem dot com
 *
 * This file is part of cvc.
 *
 * cvc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * cvc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with cvc.  If not, see <http://www.gnu.org/licenses/>.
 *
 * You can download cvc from https://github.com/d-m-bailey/cvc.git
 */

#ifndef CFIXEDTEXT_HH_
#define CFIXEDTEXT_HH_

#include "Cvc.hh"

#include "obstack.h"

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

class CFixedText {
private:
	struct obstack fixedTextObstack;
	int size = 0;
	text_t firstAddress;
public:
	CStringTextMap fixedTextToAddressMap;
	CFixedText();
	~CFixedText();
	inline int Size() {return size;};
	inline int	Entries() {return fixedTextToAddressMap.size();};
	void Clear();
	text_t BlankTextAddress() {return firstAddress;};
	text_t SetTextAddress(const text_t theText);
	text_t SetTextAddress(const string theType, CTextList* theNewTextList);
	text_t GetTextAddress(const text_t theText);
	text_t GetTextAddress(string theText);
	void Print();
	friend void DumpStatistics(unordered_map<string, text_t> & theMap, string theTitle);
};

class CCdlText : public CFixedText {

};
#endif /* CFIXEDTEXT_HH_ */
