/*
fuzzylite (R), a fuzzy logic control library in C++.

Copyright (C) 2010-2024 FuzzyLite Limited. All rights reserved.
Author: Juan Rada-Vilela, PhD <jcrada@fuzzylite.com>.

This file is part of fuzzylite.

fuzzylite is free software: you can redistribute it and/or modify it under
the terms of the FuzzyLite License included with the software.

You should have received a copy of the FuzzyLite License along with
fuzzylite. If not, see <https://github.com/fuzzylite/fuzzylite/>.

fuzzylite is a registered trademark of FuzzyLite Limited.
*/

#include "fuzzylite/norm/s/BoundedSum.h"

#include "fuzzylite/Operation.h"

namespace fuzzylite {

    std::string BoundedSum::className() const {
        return "BoundedSum";
    }

    Complexity BoundedSum::complexity() const {
        return Complexity().arithmetic(1).function(1);
    }

    scalar BoundedSum::compute(scalar a, scalar b) const {
        return Op::min(scalar(1.0), a + b);
    }

    BoundedSum* BoundedSum::clone() const {
        return new BoundedSum(*this);
    }

    SNorm* BoundedSum::constructor() {
        return new BoundedSum;
    }

}
