CREATE CONSTRAINT unique_enterprise_name IF NOT EXISTS
FOR (e:Enterprise) REQUIRE e.name IS UNIQUE;

CREATE CONSTRAINT unique_equipment_name IF NOT EXISTS
FOR (eq:Equipment) REQUIRE eq.name IS UNIQUE;

CREATE VECTOR INDEX equipment_embeddings IF NOT EXISTS
FOR (eq:Equipment) ON (eq.embedding)
OPTIONS {
  indexConfig: {
    `vector.dimensions`: 384,
    `vector.similarity_function`: 'cosine'
  }
};