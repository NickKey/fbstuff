/*
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Initial Developer of the Original Code is Adriano dos Santos Fernandes.
 * Portions created by the Initial Developer are Copyright (C) 2011 the Initial Developer.
 * All Rights Reserved.
 *
 * Contributor(s):
 *
 */

#include "V3Util.h"
#include <string>
#include <boost/lexical_cast.hpp>
#include <boost/assert.hpp>

namespace V3Util
{
	using namespace Firebird;
	using std::string;
	using boost::lexical_cast;

//------------------------------------------------------------------------------

IMaster* master = fb_get_master_interface();
IProvider* dispatcher = master->getDispatcher();

//--------------------------------------

void getEngineVersion(Firebird::IAttachment* attachment, unsigned* major, unsigned* minor,
	unsigned* revision)
{
	static const ISC_UCHAR ITEMS[] = {isc_info_firebird_version};
	ISC_UCHAR buffer[512];
	IStatus* status = master->getStatus();

	attachment->getInfo(status, sizeof(ITEMS), ITEMS, sizeof(buffer), buffer);
	BOOST_VERIFY(status->isSuccess());

	if (buffer[0] == isc_info_firebird_version)
	{
		const ISC_UCHAR* p = buffer + 5;

		if (major)
			*major = p[4] - '0';

		if (minor)
			*minor = p[6] - '0';

		if (revision)
			*revision = p[8] - '0';
	}
	else
	{
		if (major)
			*major = 0;

		if (minor)
			*minor = 0;

		if (revision)
			*revision = 0;
	}

	status->dispose();
}

string valueToString(IAttachment* attachment, ITransaction* transaction,
	MessageImpl& message, Offset<void*>& field)
{
	if (message.isNull(field))
		return "";

	void* p = message[field];
	string s;

	IStatus* status = master->getStatus();

	switch (message.getMetadata()->getType(status, field.index))
	{
		//// TODO: scale for SQL_SHORT, SQL_LONG and SQL_INT64

		case SQL_SHORT:
			s = lexical_cast<string>(*static_cast<ISC_SHORT*>(p));
			break;

		case SQL_LONG:
			s = lexical_cast<string>(*static_cast<ISC_LONG*>(p));
			break;

		case SQL_INT64:
			s = lexical_cast<string>(*static_cast<ISC_INT64*>(p));
			break;

		case SQL_FLOAT:
			s = lexical_cast<string>(*static_cast<float*>(p));
			break;

		case SQL_DOUBLE:
			s = lexical_cast<string>(*static_cast<double*>(p));
			break;

		case SQL_BLOB:
		{
			IBlob* blob = attachment->openBlob(status, transaction,
				static_cast<ISC_QUAD*>(p), 0, NULL);
			BOOST_VERIFY(status->isSuccess());

			string str;
			char blobBuffer[1024];
			unsigned blobLen;

			while ((blobLen = blob->getSegment(status, sizeof(blobBuffer), blobBuffer)) != 0)
				str.append(blobBuffer, blobLen);

			blob->close(status);
			BOOST_VERIFY(status->isSuccess());

			s = str;
			break;
		}

		case SQL_TEXT:
		case SQL_VARYING:
			s = string(static_cast<char*>(p) + sizeof(ISC_USHORT),
				*static_cast<ISC_USHORT*>(p));
			break;

		default:
			BOOST_VERIFY(false);
			s = "";
			break;
	}

	status->dispose();

	return s;
}

//------------------------------------------------------------------------------

}	// V3Util
