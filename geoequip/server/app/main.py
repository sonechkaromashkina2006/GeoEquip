from contextlib import asynccontextmanager
from typing import List

from fastapi import FastAPI, HTTPException, Query
from neo4j import GraphDatabase

from app.config import NEO4J_URI, NEO4J_USER, NEO4J_PASSWORD
from app.database import init_db, close_db, get_driver
from app.models import (
    SearchRequest, 
    EquipmentSearchItem, 
    DatabaseObjectItem, 
    SimilarEnterprise
)
from app.services.search_service import search, vector_search_raw, model

@asynccontextmanager
async def lifespan(app: FastAPI):
    await init_db()
    yield
    await close_db()

app = FastAPI(title="GeoEquip Backend API", lifespan=lifespan)

@app.get("/similar/{enterprise_name}", response_model=List[SimilarEnterprise])
async def get_similar_enterprises(enterprise_name: str):
    driver = get_driver()

    cypher_query = """
    MATCH (e1:Enterprise {name: $name})-[:HAS_EQUIPMENT]->
          (:Equipment)-[:BELONGS_TO]->(c:Category)
          <-[:BELONGS_TO]-(:Equipment)<-[:HAS_EQUIPMENT]-(e2:Enterprise)
    WHERE e1 <> e2
    WITH e2, count(DISTINCT c) AS shared_categories
    OPTIONAL MATCH (e1)-[:HAS_EQUIPMENT]->(:Equipment)-[:SUITABLE_FOR]->(t:Task)
          <-[:SUITABLE_FOR]-(:Equipment)<-[:HAS_EQUIPMENT]-(e2)
    WITH e2, shared_categories, count(DISTINCT t) AS shared_tasks
    RETURN 
        e2.name AS name,
        toFloat(e2.lat) AS lat,
        toFloat(e2.lon) AS lon,
        shared_categories,
        shared_tasks
    ORDER BY shared_categories DESC, shared_tasks DESC
    LIMIT 5
    """

    async with driver.session() as session:
        result = await session.run(cypher_query, name=enterprise_name)
        records = await result.data()

    return [
        SimilarEnterprise(
            name=r["name"],
            lat=r["lat"],
            lon=r["lon"],
            shared_categories=r["shared_categories"],
            shared_tasks=r["shared_tasks"]
        )
        for r in records
    ]

@app.post("/search")
def search_endpoint(request: SearchRequest):
    results = search(request.query, top_enterprises=request.top_k)
    return {"results": results}

@app.get("/search/equipment", response_model=List[EquipmentSearchItem])
def search_equipment(query: str = Query(...)):
    query_vector = model.encode(query).tolist()
    
    with GraphDatabase.driver(NEO4J_URI, auth=(NEO4J_USER, NEO4J_PASSWORD)) as drv:
        with drv.session() as session:
            raw = session.execute_read(vector_search_raw, query_vector, top_k=20)
    
    return [EquipmentSearchItem(
        name=r["equipment"],
        enterprise=r["enterprise"],
        description=r["description"],
        score=round(r["score"], 4)
    ) for r in raw]

@app.get("/api/database/all", response_model=List[DatabaseObjectItem])
async def get_all_database():
    driver = get_driver()

    cypher_query = """
    MATCH (ent:Enterprise)
    RETURN 
        ent.name AS name,
        coalesce(ent.type, 'Предприятие') AS type,
        toFloat(ent.lat) AS lat,
        toFloat(ent.lon) AS lon,
        toFloat(coalesce(ent.accessibility, 0.0)) AS accessibility
    """

    async with driver.session() as session:
        result = await session.run(cypher_query)
        records = await result.data()

    return [
        DatabaseObjectItem(
            name=record["name"],
            type=record["type"],
            lat=record["lat"],
            lon=record["lon"],
            accessibility=record["accessibility"]
        )
        for record in records
    ]