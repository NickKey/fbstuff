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
#include <boost/test/unit_test.hpp>

using namespace V3Util;
using namespace Firebird;
using std::string;

//------------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(v3api)


BOOST_AUTO_TEST_CASE(cursor)
{
	const string location = FbTest::getLocation("cursor.fdb");

	IStatus* status = master->getStatus();

	IAttachment* attachment = dispatcher->createDatabase(status, location.c_str(), 0, NULL);
	BOOST_CHECK(status->isSuccess());
	BOOST_REQUIRE(attachment);

	ITransaction* transaction = attachment->startTransaction(status, 0, NULL);
	BOOST_CHECK(status->isSuccess());
	BOOST_REQUIRE(transaction);

	// Create some tables.
	{
		attachment->execute(status, transaction, 0,
			"create table t (n integer)", FbTest::DIALECT, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		transaction->commitRetaining(status);
		BOOST_CHECK(status->isSuccess());
	}

	attachment->execute(status, transaction, 0,
		"insert into t values (10)", FbTest::DIALECT, 0, NULL, NULL);
	BOOST_CHECK(status->isSuccess());

	attachment->execute(status, transaction, 0,
		"insert into t values (20)", FbTest::DIALECT, 0, NULL, NULL);
	BOOST_CHECK(status->isSuccess());

	attachment->execute(status, transaction, 0,
		"insert into t values (30)", FbTest::DIALECT, 0, NULL, NULL);
	BOOST_CHECK(status->isSuccess());

	IStatement* stmt1 = attachment->allocateStatement(status);
	BOOST_CHECK(status->isSuccess());
	BOOST_REQUIRE(stmt1);

	IStatement* stmt2 = attachment->allocateStatement(status);
	BOOST_CHECK(status->isSuccess());
	BOOST_REQUIRE(stmt2);

	IStatement* stmt3 = attachment->allocateStatement(status);
	BOOST_CHECK(status->isSuccess());
	BOOST_REQUIRE(stmt3);

	IStatement* stmt4 = attachment->allocateStatement(status);
	BOOST_CHECK(status->isSuccess());
	BOOST_REQUIRE(stmt4);

	IStatement* stmt5 = attachment->allocateStatement(status);
	BOOST_CHECK(status->isSuccess());
	BOOST_REQUIRE(stmt5);

	stmt1->prepare(status, transaction, 0, "select n from t order by n for update",
		FbTest::DIALECT, 0);
	BOOST_CHECK(status->isSuccess());

	stmt1->setCursorName(status, "C");
	BOOST_CHECK(status->isSuccess());

	stmt2->prepare(status, transaction, 0,
		"execute block returns (n1 integer, n2 integer, n3 integer, n4 integer)\n"
		"as\n"
		"  declare c cursor for (select n from t order by n);\n"
		"  declare v integer;\n"
		"begin\n"
		"  open c;\n"
		"  fetch c into v;\n"
		"  update t set n = n * 10 where current of c\n"
		"    returning n, old.n, new.n, t.n into n1, n2, n3, n4;\n"
		"  suspend;\n"
		"  fetch c into v;\n"
		"  fetch c into v;\n"
		"  delete from t where current of c\n"
		"    returning n, t.n, -1, -1 into n1, n2, n3, n4;\n"
		"  suspend;\n"
		"  close c;\n"
		"end",
		FbTest::DIALECT, 0);
	BOOST_CHECK(status->isSuccess());

	stmt3->prepare(status, transaction, 0,
		"update t set n = n * 10 where current of c returning n, old.n, new.n, t.n",
		FbTest::DIALECT, 0);
	BOOST_CHECK(status->isSuccess());

	stmt4->prepare(status, transaction, 0,
		"update t set n = n * 10 where n = 200 returning n, old.n, new.n, t.n",
		FbTest::DIALECT, 0);
	BOOST_CHECK(status->isSuccess());

	stmt5->prepare(status, transaction, 0,
		"delete from t where current of c returning n, t.n, -1, -1",
		FbTest::DIALECT, 0);
	BOOST_CHECK(status->isSuccess());

	FB_MESSAGE_DESC(OutputType1,
		(FB_INTEGER, n)
	) output1;

	FB_MESSAGE_DESC(OutputType2,
		(FB_INTEGER, n1)
		(FB_INTEGER, n2)
		(FB_INTEGER, n3)
		(FB_INTEGER, n4)
	) output2;

	{
		output2.n1 = output2.n2 = output2.n3 = output2.n4 = 0;

		stmt1->execute(status, transaction, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		BOOST_CHECK(stmt1->fetch(status, &output1.desc) != 100);
		BOOST_CHECK(output1.n == 10);

		stmt3->execute(status, transaction, 0, NULL, &output2.desc);
		BOOST_CHECK(status->isSuccess());
		BOOST_CHECK(output2.n1 == 100 && output2.n2 == 10 && output2.n3 == 100 && output2.n4 == 100);

		BOOST_CHECK(stmt1->fetch(status, &output1.desc) != 100);
		BOOST_CHECK(output1.n == 20);

		stmt3->execute(status, transaction, 0, NULL, &output2.desc);
		BOOST_CHECK(status->isSuccess());
		BOOST_CHECK(output2.n1 == 200 && output2.n2 == 20 && output2.n3 == 200 && output2.n4 == 200);

		BOOST_CHECK(stmt1->fetch(status, &output1.desc) != 100);
		BOOST_CHECK(output1.n == 30);

		stmt5->execute(status, transaction, 0, NULL, &output2.desc);
		BOOST_CHECK(status->isSuccess());
		BOOST_CHECK(output2.n1 == 30 && output2.n2 == 30);

		BOOST_CHECK(stmt1->fetch(status, &output1.desc) == 100);

		stmt1->free(status, DSQL_close);
		BOOST_CHECK(status->isSuccess());
	}

	{
		stmt2->execute(status, transaction, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		BOOST_CHECK(stmt2->fetch(status, &output2.desc) != 100);
		BOOST_CHECK(output2.n1 == 1000 && output2.n2 == 100 && output2.n3 == 1000 && output2.n4 == 1000);

		stmt2->free(status, DSQL_close);
		BOOST_CHECK(status->isSuccess());
	}

	stmt4->execute(status, transaction, 0, NULL, &output2.desc);
	BOOST_CHECK(status->isSuccess());
	BOOST_CHECK(output2.n1 == 2000 && output2.n2 == 200 && output2.n3 == 2000 && output2.n4 == 2000);

	stmt4->execute(status, transaction, 0, NULL, &output2.desc);
	BOOST_CHECK(status->isSuccess());
	BOOST_CHECK(output2.n1 == 0 && output2.n2 == 0 && output2.n3 == 0 && output2.n4 == 0);

	{
		stmt1->execute(status, transaction, 0, NULL, NULL);
		BOOST_CHECK(status->isSuccess());

		BOOST_CHECK(stmt1->fetch(status, &output1.desc) != 100);
		BOOST_CHECK(output1.n == 1000);

		BOOST_CHECK(stmt1->fetch(status, &output1.desc) != 100);
		BOOST_CHECK(output1.n == 2000);

		BOOST_CHECK(stmt1->fetch(status, &output1.desc) == 100);

		stmt1->free(status, DSQL_close);
		BOOST_CHECK(status->isSuccess());
	}

	stmt4->free(status, DSQL_drop);
	BOOST_CHECK(status->isSuccess());

	stmt3->free(status, DSQL_drop);
	BOOST_CHECK(status->isSuccess());

	stmt2->free(status, DSQL_drop);
	BOOST_CHECK(status->isSuccess());

	stmt1->free(status, DSQL_drop);
	BOOST_CHECK(status->isSuccess());

	transaction->commit(status);
	BOOST_CHECK(status->isSuccess());

	attachment->drop(status);
	BOOST_CHECK(status->isSuccess());

	status->dispose();
}


BOOST_AUTO_TEST_SUITE_END()
