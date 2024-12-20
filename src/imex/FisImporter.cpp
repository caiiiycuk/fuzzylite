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

#include "fuzzylite/imex/FisImporter.h"

#include "fuzzylite/Headers.h"

namespace fuzzylite {

    FisImporter::FisImporter() : Importer() {}

    FisImporter::~FisImporter() {}

    std::string FisImporter::name() const {
        return "FisImporter";
    }

    Engine* FisImporter::fromString(const std::string& fis) const {
        FL_unique_ptr<Engine> engine(new Engine);

        std::istringstream fisReader(fis);
        std::string line;
        std::size_t lineNumber = 0;

        std::vector<std::string> sections;
        while (std::getline(fisReader, line)) {
            ++lineNumber;
            // remove comments
            line = Op::split(line, "//", false).front();
            line = Op::split(line, "#", false).front();
            line = Op::trim(line);
            if (line.empty() or line.at(0) == '%')
                continue;

            line = Op::findReplace(line, "'", "");

            if ("[System]" == line.substr(0, std::string("[System]").size())
                or "[Input" == line.substr(0, std::string("[Input").size())
                or "[Output" == line.substr(0, std::string("[Output").size())
                or "[Rules]" == line.substr(0, std::string("[Rules]").size())) {
                sections.push_back(line);
            } else if (not sections.empty()) {
                sections.at(sections.size() - 1) += "\n" + line;
            } else {
                std::ostringstream ss;
                ss << "[import error] line " << lineNumber << " <" << line
                   << "> "
                      "does not belong to any section";
                throw Exception(ss.str(), FL_AT);
            }
        }
        std::string andMethod, orMethod, impMethod, aggMethod, defuzzMethod;
        for (std::size_t i = 0; i < sections.size(); ++i)
            if ("[System]" == sections.at(i).substr(0, std::string("[System]").size()))
                importSystem(sections.at(i), engine.get(), andMethod, orMethod, impMethod, aggMethod, defuzzMethod);
            else if ("[Input" == sections.at(i).substr(0, std::string("[Input").size()))
                importInput(sections.at(i), engine.get());
            else if ("[Output" == sections.at(i).substr(0, std::string("[Output").size()))
                importOutput(sections.at(i), engine.get());
            else if ("[Rules]" == sections.at(i).substr(0, std::string("[Rules]").size()))
                importRules(sections.at(i), engine.get());
            else
                throw Exception("[import error] section <" + sections.at(i) + "> not recognized", FL_AT);
        engine->configure(
            translateTNorm(andMethod),
            translateSNorm(orMethod),
            translateTNorm(impMethod),
            translateSNorm(aggMethod),
            translateDefuzzifier(defuzzMethod),
            General().className()
        );
        return engine.release();
    }

    void FisImporter::importSystem(
        const std::string& section,
        Engine* engine,
        std::string& andMethod,
        std::string& orMethod,
        std::string& impMethod,
        std::string& aggMethod,
        std::string& defuzzMethod
    ) const {
        std::istringstream reader(section);
        std::string line;
        std::getline(reader, line);  // ignore first line [System]
        while (std::getline(reader, line)) {
            std::vector<std::string> keyValue = Op::split(line, "=");

            std::string key = Op::trim(keyValue.at(0));
            std::string value;
            for (std::size_t i = 1; i < keyValue.size(); ++i)
                value += keyValue.at(i);
            value = Op::trim(value);
            if (key == "Name")
                engine->setName(value);
            else if (key == "AndMethod")
                andMethod = value;
            else if (key == "OrMethod")
                orMethod = value;
            else if (key == "ImpMethod")
                impMethod = value;
            else if (key == "AggMethod")
                aggMethod = value;
            else if (key == "DefuzzMethod")
                defuzzMethod = value;
            else if (key == "Type" or key == "Version" or key == "NumInputs" or key == "NumOutputs" or key == "NumRules"
                     or key == "NumMFs") {
                // ignore because are redundant.
            } else
                throw Exception("[import error] token <" + key + "> not recognized", FL_AT);
        }
    }

    void FisImporter::importInput(const std::string& section, Engine* engine) const {
        std::istringstream reader(section);
        std::string line;
        std::getline(reader, line);  // ignore first line [Input#]

        InputVariable* input = new InputVariable;
        engine->addInputVariable(input);

        while (std::getline(reader, line)) {
            std::vector<std::string> keyValue = Op::split(line, "=");
            if (keyValue.size() != 2)
                throw Exception(
                    "[syntax error] expected a property of type "
                    "'key=value', but found <"
                        + line + ">",
                    FL_AT
                );
            std::string key = Op::trim(keyValue.at(0));
            std::string value = Op::trim(keyValue.at(1));

            if (key == "Name") {
                input->setName(Op::validName(value));
            } else if (key == "Enabled") {
                input->setEnabled(Op::isEq(Op::toScalar(value), 1.0));
            } else if (key == "Range") {
                std::pair<scalar, scalar> minmax = parseRange(value);
                input->setMinimum(minmax.first);
                input->setMaximum(minmax.second);
            } else if (key.substr(0, 2) == "MF") {
                input->addTerm(parseTerm(value, engine));
            } else if (key == "NumMFs") {
                // ignore
            } else {
                throw Exception("[import error] token <" + key + "> not recognized", FL_AT);
            }
        }
    }

    void FisImporter::importOutput(const std::string& section, Engine* engine) const {
        std::istringstream reader(section);
        std::string line;
        std::getline(reader, line);  // ignore first line [Output#]

        OutputVariable* output = new OutputVariable;
        engine->addOutputVariable(output);

        while (std::getline(reader, line)) {
            std::vector<std::string> keyValue = Op::split(line, "=");
            if (keyValue.size() != 2)
                throw Exception(
                    "[syntax error] expected a property of type "
                    "'key=value', but found < "
                        + line + ">",
                    FL_AT
                );
            std::string key = Op::trim(keyValue.at(0));
            std::string value = Op::trim(keyValue.at(1));

            if (key == "Name") {
                output->setName(Op::validName(value));
            } else if (key == "Enabled") {
                output->setEnabled(Op::isEq(Op::toScalar(value), 1.0));
            } else if (key == "Range") {
                std::pair<scalar, scalar> minmax = parseRange(value);
                output->setMinimum(minmax.first);
                output->setMaximum(minmax.second);
            } else if (key.substr(0, 2) == "MF") {
                output->addTerm(parseTerm(value, engine));
            } else if (key == "Default") {
                output->setDefaultValue(Op::toScalar(value));
            } else if (key == "LockPrevious") {
                output->setLockPreviousValue(Op::isEq(Op::toScalar(value), 1.0));
            } else if (key == "LockRange") {
                output->setLockValueInRange(Op::isEq(Op::toScalar(value), 1.0));
            } else if (key == "NumMFs") {
                // ignore
            } else {
                throw Exception("[import error] token <" + key + "> not recognized", FL_AT);
            }
        }
    }

    void FisImporter::importRules(const std::string& section, Engine* engine) const {
        std::istringstream reader(section);
        std::string line;
        std::getline(reader, line);  // ignore first line [Rules]

        RuleBlock* ruleblock = new RuleBlock;
        engine->addRuleBlock(ruleblock);

        while (std::getline(reader, line)) {
            std::vector<std::string> inputsAndRest = Op::split(line, ",");
            if (inputsAndRest.size() != 2)
                throw Exception(
                    "[syntax error] expected rule to match pattern "
                    "<'i '+, 'o '+ (w) : '1|2'>, but found instead <"
                        + line + ">",
                    FL_AT
                );

            std::vector<std::string> outputsAndRest = Op::split(inputsAndRest.at(1), ":");
            if (outputsAndRest.size() != 2)
                throw Exception(
                    "[syntax error] expected rule to match pattern "
                    "<'i '+, 'o '+ (w) : '1|2'>, but found instead <"
                        + line + ">",
                    FL_AT
                );

            std::vector<std::string> inputs = Op::split(inputsAndRest.at(0), " ");
            std::vector<std::string> outputs = Op::split(outputsAndRest.at(0), " ");
            std::string weightInParenthesis = outputs.at(outputs.size() - 1);
            outputs.erase(outputs.begin() + outputs.size() - 1);
            std::string connector = Op::trim(outputsAndRest.at(1));

            if (inputs.size() != engine->numberOfInputVariables()) {
                std::ostringstream ss;
                ss << "[syntax error] expected <" << engine->numberOfInputVariables()
                   << ">"
                      " input variables, but found <"
                   << inputs.size()
                   << ">"
                      " input variables in rule <"
                   << line << ">";
                throw Exception(ss.str(), FL_AT);
            }
            if (outputs.size() != engine->numberOfOutputVariables()) {
                std::ostringstream ss;
                ss << "[syntax error] expected <" << engine->numberOfOutputVariables()
                   << ">"
                      " output variables, but found <"
                   << outputs.size()
                   << ">"
                      " output variables in rule <"
                   << line << ">";
                throw Exception(ss.str(), FL_AT);
            }

            std::vector<std::string> antecedent, consequent;

            for (std::size_t i = 0; i < inputs.size(); ++i) {
                scalar inputCode = Op::toScalar(inputs.at(i));
                if (Op::isEq(inputCode, 0.0))
                    continue;
                std::ostringstream ss;
                ss << engine->getInputVariable(i)->getName() << " " << Rule::isKeyword() << " "
                   << translateProposition(inputCode, engine->getInputVariable(i));
                antecedent.push_back(ss.str());
            }

            for (std::size_t i = 0; i < outputs.size(); ++i) {
                scalar outputCode = Op::toScalar(outputs.at(i));
                if (Op::isEq(outputCode, 0.0))
                    continue;
                std::ostringstream ss;
                ss << engine->getOutputVariable(i)->getName() << " " << Rule::isKeyword() << " "
                   << translateProposition(outputCode, engine->getOutputVariable(i));
                consequent.push_back(ss.str());
            }

            std::ostringstream ruleText;

            ruleText << Rule::ifKeyword() << " ";
            for (std::size_t i = 0; i < antecedent.size(); ++i) {
                ruleText << antecedent.at(i);
                if (i + 1 < antecedent.size()) {
                    ruleText << " ";
                    if (connector == "1")
                        ruleText << Rule::andKeyword() << " ";
                    else if (connector == "2")
                        ruleText << Rule::orKeyword() << " ";
                    else
                        throw Exception("[syntax error] connector <" + connector + "> not recognized", FL_AT);
                }
            }

            ruleText << " " << Rule::thenKeyword() << " ";
            for (std::size_t i = 0; i < consequent.size(); ++i) {
                ruleText << consequent.at(i);
                if (i + 1 < consequent.size())
                    ruleText << " " << Rule::andKeyword() << " ";
            }

            std::ostringstream ss;
            for (std::size_t i = 0; i < weightInParenthesis.size(); ++i) {
                if (weightInParenthesis.at(i) == '(' or weightInParenthesis.at(i) == ')'
                    or weightInParenthesis.at(i) == ' ')
                    continue;
                ss << weightInParenthesis.at(i);
            }

            scalar weight = Op::toScalar(ss.str());
            if (not Op::isEq(weight, 1.0))
                ruleText << " " << Rule::withKeyword() << " " << Op::str(weight);
            Rule* rule = new Rule(ruleText.str());
            try {
                rule->load(engine);
            } catch (...) {
                // ignore
            }
            ruleblock->addRule(rule);
        }
    }

    std::string FisImporter::translateProposition(scalar code, Variable* variable) const {
        int intPart = (int)std::floor(std::abs(code)) - 1;
        scalar fracPart = std::fmod(std::abs(code), scalar(1.0));
        if (intPart >= static_cast<int>(variable->numberOfTerms())) {
            std::ostringstream ex;
            ex << "[syntax error] the code <" << code
               << "> refers to a term "
                  "out of range from variable <"
               << variable->getName() << ">";
            throw Exception(ex.str(), FL_AT);
        }

        bool isAny = intPart < 0;
        std::ostringstream ss;
        if (code < 0)
            ss << Not().name() << " ";
        if (Op::isEq(fracPart, 0.01))
            ss << Seldom().name() << " ";
        else if (Op::isEq(fracPart, 0.05))
            ss << Somewhat().name() << " ";
        else if (Op::isEq(fracPart, 0.2))
            ss << Very().name() << " ";
        else if (Op::isEq(fracPart, 0.3))
            ss << Extremely().name() << " ";
        else if (Op::isEq(fracPart, 0.4))
            ss << Very().name() << " " << Very().name() << " ";
        else if (Op::isEq(fracPart, 0.99))
            ss << Any().name() << " ";
        else if (not Op::isEq(fracPart, 0.0))
            throw Exception("[syntax error] no hedge defined in FIS format for <" + Op::str(fracPart) + ">", FL_AT);
        if (not isAny)
            ss << variable->getTerm(intPart)->getName();
        return ss.str();
    }

    std::string FisImporter::translateTNorm(const std::string& name) const {
        if (name.empty())
            return "";
        if (name == "min")
            return Minimum().className();
        if (name == "prod")
            return AlgebraicProduct().className();
        if (name == "bounded_difference")
            return BoundedDifference().className();
        if (name == "drastic_product")
            return DrasticProduct().className();
        if (name == "einstein_product")
            return EinsteinProduct().className();
        if (name == "hamacher_product")
            return HamacherProduct().className();
        if (name == "nilpotent_minimum")
            return NilpotentMinimum().className();
        return name;
    }

    std::string FisImporter::translateSNorm(const std::string& name) const {
        if (name.empty())
            return "";
        if (name == "max")
            return Maximum().className();
        if (name == "probor")
            return AlgebraicSum().className();
        if (name == "bounded_sum")
            return BoundedSum().className();
        if (name == "normalized_sum")
            return NormalizedSum().className();
        if (name == "drastic_sum")
            return DrasticSum().className();
        if (name == "einstein_sum")
            return EinsteinSum().className();
        if (name == "hamacher_sum")
            return HamacherSum().className();
        if (name == "nilpotent_maximum")
            return NilpotentMaximum().className();
        if (name == "sum")
            return UnboundedSum().className();
        return name;
    }

    std::string FisImporter::translateDefuzzifier(const std::string& name) const {
        if (name.empty())
            return "";
        if (name == "centroid")
            return Centroid().className();
        if (name == "bisector")
            return Bisector().className();
        if (name == "lom")
            return LargestOfMaximum().className();
        if (name == "mom")
            return MeanOfMaximum().className();
        if (name == "som")
            return SmallestOfMaximum().className();
        if (name == "wtaver")
            return WeightedAverage().className();
        if (name == "wtsum")
            return WeightedSum().className();
        return name;
    }

    std::pair<scalar, scalar> FisImporter::parseRange(const std::string& range) const {
        std::vector<std::string> parts = Op::split(range, " ");
        if (parts.size() != 2)
            throw Exception(
                "[syntax error] expected range in format '[begin end]',"
                " but found <"
                    + range + ">",
                FL_AT
            );
        std::string begin = parts.at(0), end = parts.at(1);
        if (begin.at(0) != '[' or end.at(end.size() - 1) != ']')
            throw Exception(
                "[syntax error] expected range in format '[begin end]',"
                " but found <"
                    + range + ">",
                FL_AT
            );
        std::pair<scalar, scalar> result;
        result.first = Op::toScalar(begin.substr(1));
        result.second = Op::toScalar(end.substr(0, end.size() - 1));
        return result;
    }

    Term* FisImporter::parseTerm(const std::string& fis, const Engine* engine) const {
        std::ostringstream ss;
        for (std::size_t i = 0; i < fis.size(); ++i)
            if (not(fis.at(i) == '[' or fis.at(i) == ']'))
                ss << fis.at(i);
        std::string line = ss.str();

        std::vector<std::string> nameTerm = Op::split(line, ":");
        if (nameTerm.size() != 2) {
            throw Exception(
                "[syntax error] expected term in format 'name':'class',[params], "
                "but found <"
                    + line + ">",
                FL_AT
            );
        }
        std::vector<std::string> termParams = Op::split(nameTerm.at(1), ",");
        if (termParams.size() != 2) {
            throw Exception(
                "[syntax error] expected term in format 'name':'class',[params], "
                "but found "
                    + line,
                FL_AT
            );
        }

        std::vector<std::string> parameters = Op::split(termParams.at(1), " ");
        for (std::size_t i = 0; i < parameters.size(); ++i)
            parameters.at(i) = Op::trim(parameters.at(i));
        return createInstance(Op::trim(termParams.at(0)), Op::trim(nameTerm.at(0)), parameters, engine);
    }

    Term* FisImporter::createInstance(
        const std::string& mClass, const std::string& name, const std::vector<std::string>& params, const Engine* engine
    ) const {
        std::map<std::string, std::string> mapping;
        mapping["binarymf"] = Binary().className();
        mapping["concavemf"] = Concave().className();
        mapping["constant"] = Constant().className();
        mapping["cosinemf"] = Cosine().className();
        mapping["discretemf"] = Discrete().className();
        mapping["function"] = Function().className();
        mapping["gbellmf"] = Bell().className();
        mapping["gaussmf"] = Gaussian().className();
        mapping["gauss2mf"] = GaussianProduct().className();
        mapping["linear"] = Linear().className();
        mapping["pimf"] = PiShape().className();
        mapping["rampmf"] = Ramp().className();
        mapping["rectmf"] = Rectangle().className();
        mapping["smf"] = SShape().className();
        mapping["sigmf"] = Sigmoid().className();
        mapping["dsigmf"] = SigmoidDifference().className();
        mapping["psigmf"] = SigmoidProduct().className();
        mapping["spikemf"] = Spike().className();
        mapping["trapmf"] = Trapezoid().className();
        mapping["trimf"] = Triangle().className();
        mapping["zmf"] = ZShape().className();

        std::vector<std::string> sortedParams = params;

        if (mClass == "gbellmf" and params.size() >= 3) {
            sortedParams.at(0) = params.at(2);
            sortedParams.at(1) = params.at(0);
            sortedParams.at(2) = params.at(1);
        } else if (mClass == "gaussmf" and params.size() >= 2) {
            sortedParams.at(0) = params.at(1);
            sortedParams.at(1) = params.at(0);
        } else if (mClass == "gauss2mf" and params.size() >= 4) {
            sortedParams.at(0) = params.at(1);
            sortedParams.at(1) = params.at(0);
            sortedParams.at(2) = params.at(3);
            sortedParams.at(3) = params.at(2);
        } else if (mClass == "sigmf" and params.size() >= 2) {
            sortedParams.at(0) = params.at(1);
            sortedParams.at(1) = params.at(0);
        } else if (mClass == "dsigmf" and params.size() >= 4) {
            sortedParams.at(0) = params.at(1);
            sortedParams.at(1) = params.at(0);
            sortedParams.at(2) = params.at(2);
            sortedParams.at(3) = params.at(3);
        } else if (mClass == "psigmf" and params.size() >= 4) {
            sortedParams.at(0) = params.at(1);
            sortedParams.at(1) = params.at(0);
            sortedParams.at(2) = params.at(2);
            sortedParams.at(3) = params.at(3);
        }

        std::string flClass;
        std::map<std::string, std::string>::const_iterator it = mapping.find(mClass);
        if (it != mapping.end())
            flClass = it->second;
        else
            flClass = mClass;

        FL_unique_ptr<Term> term;
        term.reset(FactoryManager::instance()->term()->constructObject(flClass));
        term->updateReference(engine);
        term->setName(Op::validName(name));
        std::string separator;
        if (not dynamic_cast<Function*>(term.get()))
            separator = " ";
        term->configure(Op::join(sortedParams, separator));
        return term.release();
    }

    FisImporter* FisImporter::clone() const {
        return new FisImporter(*this);
    }

}
