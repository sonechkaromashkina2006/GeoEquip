from typing import List, Optional
from pydantic import BaseModel

class SearchRequest(BaseModel):
    query: str
    top_k: int = 5
    accessibility: Optional[float] = 0.5
    cost: Optional[float] = 0.5

class EquipmentSearchItem(BaseModel):
    name: str
    enterprise: str
    description: str
    score: float

class DatabaseObjectItem(BaseModel):
    name: str
    type: str
    lat: float
    lon: float
    accessibility: float

class SimilarEnterprise(BaseModel):
    name: str
    lat: float
    lon: float
    shared_categories: int
    shared_tasks: int