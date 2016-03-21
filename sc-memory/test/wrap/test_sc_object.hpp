/*
 * This source file is part of an OSTIS project. For the latest info, see http://ostis.net
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#pragma once

#include "wrap/sc_addr.hpp"
#include "wrap/sc_object.hpp"

#include "test_sc_object.generated.hpp"

namespace n1
{
    namespace n2
    {
        SC_CLASS()
        class TestObject : public ScObject
        {
            SC_GENERATED_BODY()

        public:
            /// TODO: autogenerate code to call _initInternal
            TestObject() { _initInternal(); } /// TEST


        public:

            //SC_PROPERTY(Keynode, SysIdtf = "test_keynode1")
            SC_PROPERTY(Keynode, SysIdtf("test_keynode1"))
            ScAddr mTestKeynode1;

            SC_PROPERTY(Keynode, SysIdtf("test_keynode2"))
            ScAddr mTestKeynode2;
        };
    }
}