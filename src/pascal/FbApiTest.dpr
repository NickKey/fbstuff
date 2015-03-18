{
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
 * Portions created by the Initial Developer are Copyright (C) 2015 the Initial Developer.
 * All Rights Reserved.
 *
 * Contributor(s):
}

program FbApiTest;

{$ifndef FPC}
{$APPTYPE CONSOLE}
{$endif}

uses FbApi;

function fb_get_master_interface: Master; cdecl; external 'fbclient';

type
	InMessage = record
		n: SmallInt;
		nNull: WordBool;
	end;

	OutMessage = record
		relationId: SmallInt;
		relationIdNull: WordBool;
		relationName: array[0..93] of AnsiChar;
		relationNameNull: WordBool;
	end;

var
	master: FbApi.Master;
	util: FbApi.Util;
	status: FbApi.Status;
	dispatcher: Provider;
	attachment: FbApi.Attachment;
	transaction: FbApi.Transaction;
	stmt: Statement;
	inMetadata, outMetadata: MessageMetadata;
	inBuffer: InMessage;
	outBuffer: OutMessage;
	rs: ResultSet;
	s: AnsiString;
begin
	try
		master := fb_get_master_interface();
		util := master.getUtilInterface();
		status := master.getStatus();
		dispatcher := master.getDispatcher();

		attachment := dispatcher.createDatabase(status, '/tmp/pascal.fdb', 0, nil);
		transaction := attachment.startTransaction(status, 0, nil);

		stmt := attachment.prepare(status, transaction, 0,
				'select rdb$relation_id relid, rdb$relation_name csname ' +
				'  from rdb$relations ' +
				'  where rdb$relation_id < ?',
				3, 0);

		s := stmt.getPlan(status, false);
		WriteLn('Plan: ', s);
		WriteLn;

		s := stmt.getPlan(status, true);
		WriteLn('Detailed plan: ', s);
		WriteLn;

		inMetadata := stmt.getInputMetadata(status);
		outMetadata := stmt.getOutputMetadata(status);

		// It's not good to:
		// - assume metadata descriptions matches our buffer types
		// - work with CHAR (SQL_TEXT) descriptors

		inBuffer.nNull := false;
		inBuffer.n := 10;
		rs := stmt.openCursor(status, transaction, inMetadata, @inBuffer, outMetadata, 0);

		while (rs.fetchNext(status, @outBuffer) = FbApi.Status.RESULT_OK)
		do
		begin
			outBuffer.relationName[92] := #0;	// workaround: null terminate it
			WriteLn('Fetch: ', outBuffer.relationId, ' ', outBuffer.relationName);
		end;

		rs.release();

		inMetadata.release();
		outMetadata.release();

		stmt.free(status);

		transaction.commit(status);
		attachment.dropDatabase(status);
	except
		on e: FbException do
		begin
			SetLength(s, 2000);
			SetLength(s, util.formatStatus(PAnsiChar(s), 2000, e.getStatus()));
			WriteLn('Exception: ', AnsiString(s));
		end
	end
end.
