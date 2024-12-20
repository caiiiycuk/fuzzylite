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

#ifndef FL_HAMACHERPRODUCT_H
#define FL_HAMACHERPRODUCT_H

#include "fuzzylite/norm/TNorm.h"

namespace fuzzylite {

    /**
      The HamacherProduct class is a TNorm that computes the Hamacher product
      of any two values.

      @image html T-HamacherProduct.svg

      @author Juan Rada-Vilela, Ph.D.
      @see HamacherSum
      @see TNorm
      @see TNormFactory
      @see Norm
      @since 4.0
     */
    class FL_API HamacherProduct FL_IFINAL : public TNorm {
      public:
        std::string className() const FL_IOVERRIDE;

        /**
          Computes the Hamacher product of two membership function values
          @param a is a membership function value
          @param b is a membership function value
          @return @f$(a \times b) / (a+b- a \times b)@f$
         */
        scalar compute(scalar a, scalar b) const FL_IOVERRIDE;
        HamacherProduct* clone() const FL_IOVERRIDE;

        static TNorm* constructor();
    };
}
#endif /* FL_HAMACHERPRODUCT_H */
