#ifndef VBO_CLASS_H
#define VBO_CLASS_H

//#include<glad/gl.h>
#include <vector>


class VBO
{
public:
	// Reference ID of the Vertex Buffer Object
	GLuint ID;
	// Constructor that generates a Vertex Buffer Object and links it to vertices
	VBO(const std::vector<Vertex>& vertices);
    VBO(GLfloat* vertices, GLfloat size);

	// Binds the VBO
	void Bind();
	// Unbinds the VBO
	void Unbind();
	// Deletes the VBO
	void Delete();
};

VBO::VBO(GLfloat* vertices, GLfloat size)
{
    glGenBuffers(1, &ID);
    glBindBuffer(GL_ARRAY_BUFFER, ID);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}


// Constructor that generates a Vertex Buffer Object and links it to vertices
VBO::VBO(const std::vector<Vertex>& vertices)
{
    glGenBuffers(1, &ID); // Generar un identificador de buffer
    glBindBuffer(GL_ARRAY_BUFFER, ID); // Vincular el buffer GL_ARRAY_BUFFER al ID generado
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW); // Cargar los datos en el buffer
}

// Binds the VBO
void VBO::Bind()
{
	glBindBuffer(GL_ARRAY_BUFFER, ID);
}

// Unbinds the VBO
void VBO::Unbind()
{
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

// Deletes the VBO
void VBO::Delete()
{
	glDeleteBuffers(1, &ID);
}



#endif
