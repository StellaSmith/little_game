#!/bin/sh

exec conan build . -pr:h default -s:h compiler.cppstd=20 -b missing
