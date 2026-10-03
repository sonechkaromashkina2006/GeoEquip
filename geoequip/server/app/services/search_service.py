from collections import defaultdict
from neo4j import GraphDatabase
from sentence_transformers import SentenceTransformer
from app.config import NEO4J_URI, NEO4J_USER, NEO4J_PASSWORD

# Загрузка модели SentenceTransformer
model = SentenceTransformer('paraphrase-multilingual-MiniLM-L12-v2', local_files_only=True)

def vector_search_raw(tx, query_vector, top_k=10):
    query = """
    CALL db.index.vector.queryNodes('equipment_embeddings', $top_k, $vector)
    YIELD node, score
    MATCH (e:Enterprise)-[:HAS_EQUIPMENT]->(node)
    RETURN 
        e.name        AS enterprise,
        e.lat         AS lat,
        e.lon         AS lon,
        e.accessibility AS accessibility,
        e.cost        AS cost,
        node.name     AS equipment,
        node.description AS description,
        score
    """
    result = tx.run(query, top_k=top_k, vector=query_vector)
    return [record.data() for record in result]

def aggregate_by_enterprise(raw_results):
    enterprises = defaultdict(lambda: {
        "score": 0.0,
        "matched_equipment": [],
        "lat": None, "lon": None,
        "accessibility": None, "cost": None
    })

    for row in raw_results:
        name = row["enterprise"]
        if row["score"] > enterprises[name]["score"]:
            enterprises[name]["score"] = row["score"]
        enterprises[name]["matched_equipment"].append({
            "name": row["equipment"],
            "description": row["description"],
            "score": round(row["score"], 4)
        })
        enterprises[name]["lat"] = row["lat"]
        enterprises[name]["lon"] = row["lon"]
        enterprises[name]["accessibility"] = row["accessibility"]
        enterprises[name]["cost"] = row["cost"]

    sorted_result = sorted(
        [{"enterprise": k, **v} for k, v in enterprises.items()],
        key=lambda x: x["score"],
        reverse=True
    )
    return sorted_result

def search(user_query: str, top_enterprises: int = 5):
    query_vector = model.encode(user_query).tolist()

    try:
        driver = GraphDatabase.driver(NEO4J_URI, auth=(NEO4J_USER, NEO4J_PASSWORD))
        driver.verify_connectivity()
    except Exception as e:
        print(f"Ошибка подключения к Neo4j: {e}")
        return []

    with driver.session() as session:
        raw = session.execute_read(vector_search_raw, query_vector, top_k=top_enterprises * 3)

    driver.close()
    aggregated = aggregate_by_enterprise(raw)
    return aggregated[:top_enterprises]