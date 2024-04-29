#include "trianglemesh.h"

// Constructor of a triangle mesh.
TriangleMesh::TriangleMesh()
{
	// -------------------------------------------------------
	// Add your initialization code here.
	// -------------------------------------------------------
	numVertices = 0;
	numTriangles = 0;
	objCenter = glm::vec3(0.0f, 0.0f, 0.0f);
	objExtent = glm::vec3(0.0f, 0.0f, 0.0f);
	vboId = 0;
}

// Destructor of a triangle mesh.
TriangleMesh::~TriangleMesh()
{
	// -------------------------------------------------------
	// Add your release code here.
	// -------------------------------------------------------
	vertices.clear();
	subMeshes.clear();
	materials.clear();
	vertexIndices.clear();
	faceDataVertices.clear();
	positions.clear();
	texcoords.clear();
	normals.clear();
	glDeleteBuffers(1, &vboId);
	for (int i = 0; i < subMeshes.size(); i++)
		glDeleteBuffers(1, &subMeshes[i].iboId);
}

// Load the geometry and material data from an OBJ file.
bool TriangleMesh::LoadFromFile(const std::string& filePath, const bool normalized)
{	
	// Parse the OBJ file.
	// ---------------------------------------------------------------------------
    // Add your implementation here (HW1 + read *.MTL).
    // ---------------------------------------------------------------------------
	std::ifstream inputFile;
	inputFile.open(filePath);

	if (!inputFile.is_open())
	{
		std::cout << "Error loading file." << std::endl;
		exit(EXIT_FAILURE);
	}

	SubMesh* submesh = nullptr;
	int submesh_index = 0;
	unsigned int index = 0; // index of vertexPTN to be stored into vertexIndices

	std::string line;
	while (getline(inputFile, line))
	{
		std::istringstream in(line);
		std::string prefix;

		in >> prefix;

		if (prefix == "mtllib") {
			std::string materialFilePath;
			in >> materialFilePath;
			LoadFromMtlFile(materialFilePath);
		}
		else if (prefix == "v") {
			glm::vec3 v;
			in >> v.x >> v.y >> v.z;
			positions.push_back(v);
		}
		else if (prefix == "vt") {
			glm::vec2 vt;
			in >> vt.x >> vt.y;
			texcoords.push_back(vt);
		}
		else if (prefix == "vn") {
			glm::vec3 vn;
			in >> vn.x >> vn.y >> vn.z;
			normals.push_back(vn);
		}
		// new SubMesh when a different material is used
		else if (prefix == "usemtl") {
			if (submesh_index != 0) {
				subMeshes.push_back(*submesh);
				delete submesh;
			}
			submesh = new SubMesh;
			std::string mtl;;
			in >> mtl;
			submesh->material = &materials[mtl];
			submesh_index++;
		}
		else if (prefix == "f") {
			std::string s;
			int i, j, k; // file data format: P/T/N
			int num_vtx = 0; // to check the number of vertices for polygon subdivision if needed.

			while (in >> s)
			{
				sscanf_s(s.c_str(), "%d/%d/%d", &i, &j, &k);
				FaceData ptn = FaceData(i, j, k);
				num_vtx++;

				if (num_vtx > 3) // subdivide a polygon into triangles
				{
					unsigned int idx = vertexIndices[vertexIndices.size() - 3]; // add the index of the polygon's first vertex  // debug: faces > 4
					vertexIndices.push_back(idx);
					submesh->vertexIndices.push_back(idx);
					idx = vertexIndices[vertexIndices.size() - 2]; // add the index of the previous vertex 2
					vertexIndices.push_back(idx);
					submesh->vertexIndices.push_back(idx);
				}

				auto it = find(faceDataVertices.begin(), faceDataVertices.end(), ptn); // check if vertex has appeared before

				if (it == faceDataVertices.end()) // vertex not found
				{
					faceDataVertices.push_back(ptn);
					VertexPTN vtx = VertexPTN(positions[i - 1], normals[k - 1], texcoords[j - 1]); // create VertexPTN
					vertices.push_back(vtx);
					vertexIndices.push_back((unsigned int)(index));
					submesh->vertexIndices.push_back(index);
					index++;
				}
				else // vertex has appeared before
				{
					vertexIndices.push_back((unsigned int)(it - faceDataVertices.begin())); // add the corresponding vertex index
					submesh->vertexIndices.push_back((unsigned int)(it - faceDataVertices.begin()));
				}
			}
		}
	}
	subMeshes.push_back(*submesh);
	delete submesh;
	inputFile.close();

	// Normalize the geometry data.
	if (normalized) {
		// -----------------------------------------------------------------------
		// Add your normalization code here (HW1).
		// -----------------------------------------------------------------------
		float max_x, max_y, max_z;
		float min_x, min_y, min_z;
		max_x = max_y = max_z = -(std::numeric_limits<float>::infinity());
		min_x = min_y = min_z = (std::numeric_limits<float>::infinity());

		for (int i = 0; i < vertices.size(); i++) {
			max_x = std::max(max_x, vertices[i].position.x), min_x = std::min(min_x, vertices[i].position.x);
			max_y = std::max(max_y, vertices[i].position.y), min_y = std::min(min_y, vertices[i].position.y);
			max_z = std::max(max_z, vertices[i].position.z), min_z = std::min(min_z, vertices[i].position.z);
		}

		float bounding_length = std::max(max_x - min_x, std::max(max_y - min_y, max_z - min_z));

		objCenter = glm::vec3((max_x + min_x) / 2, (max_y + min_y) / 2, (max_z + min_z) / 2);
		max_x = max_y = max_z = -(std::numeric_limits<float>::infinity());
		min_x = min_y = min_z = (std::numeric_limits<float>::infinity());

		for (int i = 0; i < vertices.size(); i++)
		{
			vertices[i].position = (vertices[i].position - objCenter) / glm::vec3(bounding_length, bounding_length, bounding_length);
			max_x = std::max(max_x, vertices[i].position.x), min_x = std::min(min_x, vertices[i].position.x);
			max_y = std::max(max_y, vertices[i].position.y), min_y = std::min(min_y, vertices[i].position.y);
			max_z = std::max(max_z, vertices[i].position.z), min_z = std::min(min_z, vertices[i].position.z);
		}

		objExtent = glm::vec3(max_x - min_x, max_y - min_y, max_z - min_z);
		objCenter = glm::vec3((max_x + min_x) / 2, (max_y + min_y) / 2, (max_z + min_z) / 2);
	}

	numTriangles = vertexIndices.size() / 3;
	numVertices = vertices.size();

	CreateBuffers();

	return true;
}


bool TriangleMesh::LoadFromMtlFile(const std::string& materialFilePath)
{
	// update the correct file path
	std::size_t pos = materialFilePath.find(".");
	std::string folder = materialFilePath.substr(0, pos);
	std::string filePath = "TestModels_HW3/" + folder + "/" + materialFilePath;

	std::ifstream materialFile;
	materialFile.open(filePath);

	if (!materialFile.is_open())
	{
		std::cout << "Error loading material file." << std::endl;
		exit(EXIT_FAILURE);
	}

	std::string m_line;
	PhongMaterial mtl;
	std::string name;
	int material_index = 0;

	while (getline(materialFile, m_line)) 
	{
		std::istringstream m_in(m_line);
		std::string m_prefix;
		GLfloat x, y, z;
		std::string map_kd;

		m_in >> m_prefix;

		if (m_prefix == "newmtl") {
			if (material_index != 0) {
				materials[name] = mtl;
			}
			m_in >> name;
			mtl = PhongMaterial();
			mtl.SetName(name);
			material_index++;
		}
		else if (m_prefix == "Kd") {
			m_in >> x >> y >> z;
			mtl.SetKd(glm::vec3(x, y, z));
		}
		else if (m_prefix == "Ka") {
			m_in >> x >> y >> z;
			mtl.SetKa(glm::vec3(x, y, z));
		}
		else if (m_prefix == "Ks") {
			m_in >> x >> y >> z;
			mtl.SetKs(glm::vec3(x, y, z));
		}
		else if (m_prefix == "Ns") {
			m_in >> x;
			mtl.SetNs(x);
		}
		else if (m_prefix == "map_Kd") {
			m_in >> map_kd;
			map_kd = "TestModels_HW3/" + folder + "/" + map_kd;
			ImageTexture* map_Kd = new ImageTexture(map_kd);
			mtl.SetMapKd(map_Kd);
		}
	}
	materials[name] = mtl;
	materialFile.close();
	return true;
}

// Create vertex and index buffers.
void TriangleMesh::CreateBuffers()
{
	glGenBuffers(1, &vboId);
	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPTN) * vertices.size(), &vertices[0], GL_STATIC_DRAW);

	for (int i = 0; i < subMeshes.size(); i++) {
		glGenBuffers(1, &subMeshes[i].iboId);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subMeshes[i].iboId);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * subMeshes[i].vertexIndices.size(), &subMeshes[i].vertexIndices[0], GL_STATIC_DRAW);
	}
}

// Render each submesh.
void TriangleMesh::Render(PhongShadingDemoShaderProg* phongShadingShader)
{
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPTN), 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPTN), (const GLvoid*)12);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPTN), (const GLvoid*)24);

	for (int i = 0; i < subMeshes.size(); i++) 
	{
		if (subMeshes[i].material->GetMapKd() != nullptr) {
			subMeshes[i].material->GetMapKd()->Bind(GL_TEXTURE0);
			glUniform1i(phongShadingShader->GetLocMapKd(), 0);
			glUniform1i(phongShadingShader->GetLocFlag(), 1);
		}
		else {
			glUniform1i(phongShadingShader->GetLocFlag(), 0);
		}
		
        glUniform3fv(phongShadingShader->GetLocKa(), 1, glm::value_ptr(subMeshes[i].material->GetKa()));
        glUniform3fv(phongShadingShader->GetLocKd(), 1, glm::value_ptr(subMeshes[i].material->GetKd()));
        glUniform3fv(phongShadingShader->GetLocKs(), 1, glm::value_ptr(subMeshes[i].material->GetKs()));
        glUniform1f(phongShadingShader->GetLocNs(), subMeshes[i].material->GetNs());

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subMeshes[i].iboId);
		glDrawElements(GL_TRIANGLES, subMeshes[i].vertexIndices.size(), GL_UNSIGNED_INT, 0);
	}
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
}

// Show model information.
void TriangleMesh::ShowInfo()
{
	std::cout << "# Vertices: " << numVertices << std::endl;
	std::cout << "# Triangles: " << numTriangles << std::endl;
	std::cout << "Total " << subMeshes.size() << " subMeshes loaded" << std::endl;
	for (unsigned int i = 0; i < subMeshes.size(); ++i) {
		const SubMesh& g = subMeshes[i];
		std::cout << "SubMesh " << i << " with material: " << g.material->GetName() << std::endl;
		std::cout << "Num. triangles in the subMesh: " << g.vertexIndices.size() / 3 << std::endl;
	}
	std::cout << "Model Center: " << objCenter.x << ", " << objCenter.y << ", " << objCenter.z << std::endl;
	std::cout << "Model Extent: " << objExtent.x << " x " << objExtent.y << " x " << objExtent.z << std::endl;
}
