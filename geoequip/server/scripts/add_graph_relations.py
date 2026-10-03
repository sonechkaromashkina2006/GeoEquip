# add_graph_relations.py
from neo4j import GraphDatabase

URI = "bolt://localhost:7687" 
AUTH = ("NEO4J_USER", "NEO4J_PASSWORD") 

CATEGORIES = {
    "Лаборатория 3D-прототипирования КФУ": {
        "equipment": {
            "Промышленный 3D-принтер 3D Systems ProX SLS 600": {
                "category": "Аддитивное производство",
                "tasks": ["3D-печать", "Прототипирование", "Изготовление пресс-форм"]
            },
            "3D-сканер Creaform HandySCAN 3D": {
                "category": "Контроль геометрии",
                "tasks": ["Обратный инжиниринг", "Контроль качества"]
            }
        }
    },
    "Инжиниринговый центр ННГУ": {
        "equipment": {
            "Вертикально-фрезерный обрабатывающий центр Haas VF-2": {
                "category": "Металлообработка",
                "tasks": ["Фрезерование", "ЧПУ обработка", "Изготовление штампов"]
            },
            "Профилометр Taylor Hobson Surtronic": {
                "category": "Контроль геометрии",
                "tasks": ["Контроль качества", "Измерение шероховатости"]
            }
        }
    },
    "Самарский научный центр РАН": {
        "equipment": {
            "Сканирующий электронный микроскоп Tescan Vega": {
                "category": "Материаловедение",
                "tasks": ["Микроструктурный анализ", "Металлография"]
            },
            "Рентгеновский дифрактометр Rigaku": {
                "category": "Материаловедение",
                "tasks": ["Рентгеноструктурный анализ", "Фазовый анализ"]
            }
        }
    },
    "Технопарк «Жигулёвская долина»": {
        "equipment": {
            "Токарный станок DMG MORI CTX beta 800": {
                "category": "Металлообработка",
                "tasks": ["Токарная обработка", "ЧПУ обработка", "Точение валов"]
            },
            "Координатно-измерительная машина Hexagon": {
                "category": "Контроль геометрии",
                "tasks": ["Контроль качества", "3D измерения"]
            }
        }
    },
    "Саратовский государственный технический университет": {
        "equipment": {
            "Лазерный технологический комплекс чистки и резки": {
                "category": "Лазерные технологии",
                "tasks": ["Лазерная резка", "Очистка поверхностей"]
            },
            "Испытательная машина Instron 5982": {
                "category": "Испытания материалов",
                "tasks": ["Механические испытания", "Испытания на растяжение"]
            }
        }
    },
    "Ульяновский наноцентр ULNANOTECH": {
        "equipment": {
            "Установка плазмохимического осаждения PECVD": {
                "category": "Нанотехнологии",
                "tasks": ["Нанесение покрытий", "Тонкoplёночные технологии"]
            },
            "Атомно-силовой микроскоп NT-MDT": {
                "category": "Нанотехнологии",
                "tasks": ["Нанодиагностика", "Анализ поверхности"]
            }
        }
    },
    "Волгоградский государственный технический университет": {
        "equipment": {
            "Роботизированный сварочный комплекс KUKA": {
                "category": "Сварочные технологии",
                "tasks": ["Автоматическая сварка", "Роботизированное производство"]
            },
            "Ультразвуковой дефектоскоп Olympus": {
                "category": "Неразрушающий контроль",
                "tasks": ["Контроль качества", "Дефектоскопия"]
            }
        }
    },
    "Пензенский государственный университет": {
        "equipment": {
            "Станция пайки и ремонта BGA компонентов Ersa": {
                "category": "Электроника",
                "tasks": ["Монтаж электроники", "Ремонт плат"]
            },
            "Осциллограф Keysight InfiniiVision": {
                "category": "Электроника",
                "tasks": ["Анализ сигналов", "Отладка микроконтроллеров"]
            }
        }
    }
}

def add_relations(tx, ent_name, eq_name, category_name, tasks):
    # Создаём Category и связываем с Equipment
    tx.run("""
        MATCH (eq:Equipment {name: $eq_name})
        MERGE (c:Category {name: $category})
        MERGE (eq)-[:BELONGS_TO]->(c)
    """, eq_name=eq_name, category=category_name)

    # Создаём Task узлы и связываем с Equipment
    for task in tasks:
        tx.run("""
            MATCH (eq:Equipment {name: $eq_name})
            MERGE (t:Task {name: $task})
            MERGE (eq)-[:SUITABLE_FOR]->(t)
        """, eq_name=eq_name, task=task)

def main():
    with GraphDatabase.driver(URI, auth=AUTH) as driver:
        driver.verify_connectivity()
        with driver.session() as session:
            for ent_name, ent_data in CATEGORIES.items():
                for eq_name, eq_data in ent_data["equipment"].items():
                    session.execute_write(
                        add_relations,
                        ent_name,
                        eq_name,
                        eq_data["category"],
                        eq_data["tasks"]
                    )
                    print(f"✓ {eq_name} → {eq_data['category']}")

    print("\nГраф связей построен.")

if __name__ == "__main__":
    main()