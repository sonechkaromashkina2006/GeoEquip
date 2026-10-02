 #include "FuzzyEngine.h"
#include <fl/Headers.h>

using namespace fl;

double calculateMembership(
    const GeoObject& obj,
    double desiredEquipment,
    double desiredAccessibility,
    double desiredCost)
{
    Engine engine;
    engine.setName("GeoExpert");

    InputVariable* equipment = new InputVariable;
    equipment->setName("equipment");
    equipment->setRange(0, 100);
    equipment->addTerm(new Triangle("low", 0, 0, 50));
    equipment->addTerm(new Triangle("medium", 25, 50, 75));
    equipment->addTerm(new Triangle("high", 50, 100, 100));
    engine.addInputVariable(equipment);

    InputVariable* accessibility = new InputVariable;
    accessibility->setName("accessibility");
    accessibility->setRange(0, 100);
    accessibility->addTerm(new Triangle("low", 0, 0, 50));
    accessibility->addTerm(new Triangle("medium", 25, 50, 75));
    accessibility->addTerm(new Triangle("high", 50, 100, 100));
    engine.addInputVariable(accessibility);

    InputVariable* cost = new InputVariable;
    cost->setName("cost");
    cost->setRange(0, 100);
    cost->addTerm(new Triangle("low", 0, 0, 50));
    cost->addTerm(new Triangle("medium", 25, 50, 75));
    cost->addTerm(new Triangle("high", 50, 100, 100));
    engine.addInputVariable(cost);

    OutputVariable* suitability = new OutputVariable;
    suitability->setName("suitability");
    suitability->setRange(0, 1);
    suitability->setAggregation(new Maximum);
    suitability->setDefuzzifier(new Centroid(100));
    suitability->addTerm(new Triangle("bad", 0, 0, 0.5));
    suitability->addTerm(new Triangle("ok", 0.25, 0.5, 0.75));
    suitability->addTerm(new Triangle("good", 0.5, 1.0, 1.0));
    engine.addOutputVariable(suitability);

    RuleBlock* rules = new RuleBlock;

    rules->setConjunction(new Minimum);
    rules->setDisjunction(new Maximum);
    rules->setImplication(new Minimum);

    // 1. Лучшие сценарии - всё хорошее
    rules->addRule(Rule::parse(
        "if equipment is high and accessibility is high and cost is low then suitability is good", &engine));
    rules->addRule(Rule::parse(
        "if equipment is high and accessibility is high and cost is medium then suitability is good", &engine));

    // 2. Хорошее оборудование компенсирует среднюю доступность
    rules->addRule(Rule::parse(
        "if equipment is high and accessibility is medium and cost is low then suitability is good", &engine));
    rules->addRule(Rule::parse(
        "if equipment is high and accessibility is medium and cost is medium then suitability is good", &engine));

    // 3. Среднее оборудование требует хорошей доступности
    rules->addRule(Rule::parse(
        "if equipment is medium and accessibility is high and cost is low then suitability is good", &engine));
    rules->addRule(Rule::parse(
        "if equipment is medium and accessibility is high and cost is medium then suitability is good", &engine));

    // 4. Компромиссные ситуации - средняя пригодность
    rules->addRule(Rule::parse(
        "if equipment is high and accessibility is high and cost is high then suitability is ok", &engine));
    rules->addRule(Rule::parse(
        "if equipment is medium and accessibility is medium and cost is low then suitability is ok", &engine));
    rules->addRule(Rule::parse(
        "if equipment is medium and accessibility is medium and cost is medium then suitability is ok", &engine));
    rules->addRule(Rule::parse(
        "if equipment is low and accessibility is high and cost is low then suitability is ok", &engine));
    rules->addRule(Rule::parse(
        "if equipment is low and accessibility is high and cost is medium then suitability is ok", &engine));

    // 5. Проблемные ситуации - плохая пригодность
    rules->addRule(Rule::parse(
        "if equipment is high and accessibility is low and cost is high then suitability is bad", &engine));
    rules->addRule(Rule::parse(
        "if equipment is high and accessibility is medium and cost is high then suitability is bad", &engine));
    rules->addRule(Rule::parse(
        "if equipment is medium and accessibility is low and cost is medium then suitability is bad", &engine));
    rules->addRule(Rule::parse(
        "if equipment is medium and accessibility is low and cost is high then suitability is bad", &engine));
    rules->addRule(Rule::parse(
        "if equipment is low and accessibility is medium and cost is high then suitability is bad", &engine));
    rules->addRule(Rule::parse(
        "if equipment is low and accessibility is low and cost is low then suitability is bad", &engine));
    rules->addRule(Rule::parse(
        "if equipment is low and accessibility is low and cost is medium then suitability is bad", &engine));
    rules->addRule(Rule::parse(
        "if equipment is low and accessibility is low and cost is high then suitability is bad", &engine));

    // 6. Пограничные случаи
    rules->addRule(Rule::parse(
        "if equipment is high and accessibility is low and cost is low then suitability is ok", &engine));
    rules->addRule(Rule::parse(
        "if equipment is high and accessibility is low and cost is medium then suitability is ok", &engine));
    rules->addRule(Rule::parse(
        "if equipment is medium and accessibility is high and cost is high then suitability is ok", &engine));
    rules->addRule(Rule::parse(
        "if equipment is medium and accessibility is medium and cost is high then suitability is bad", &engine));
    rules->addRule(Rule::parse(
        "if equipment is medium and accessibility is low and cost is low then suitability is bad", &engine));
    rules->addRule(Rule::parse(
        "if equipment is low and accessibility is medium and cost is low then suitability is ok", &engine));
    rules->addRule(Rule::parse(
        "if equipment is low and accessibility is medium and cost is medium then suitability is bad", &engine));
    rules->addRule(Rule::parse(
        "if equipment is low and accessibility is high and cost is high then suitability is bad", &engine));

    engine.addRuleBlock(rules);

    double eqMatch = 100.0 - std::abs(obj.equipment - desiredEquipment);
    double accMatch = 100.0 - std::abs(obj.accessibility - desiredAccessibility);
    double costMatch = 100.0 - std::abs(obj.cost - desiredCost);

    eqMatch = std::clamp(eqMatch, 0.0, 100.0);
    accMatch = std::clamp(accMatch, 0.0, 100.0);
    costMatch = std::clamp(costMatch, 0.0, 100.0);

    equipment->setValue(eqMatch);
    accessibility->setValue(accMatch);
    cost->setValue(costMatch);

    engine.process();

    return suitability->getValue();
}

