#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string>

const GLint WIDTH = 800, HEIGHT = 800; // Константи початкової ширини та висоти вікна
GLuint shader; // Глобальні змінні для об'єктів у пам'яті відеокарти
// Структура, що об'єднує дані одного об'єкта
struct Figure {
	GLuint VAO;
	GLuint VBO;
	int verteciesAmount;
};

// Створюємо текст вершиного шейдера, який використовується для перетворення перетворення вершин у відповідні координати на екрані
static const char* vShader = R"(
	#version 330 // Вказуємо відеокарті, що код написаний на мові GLSL версії 3.3
	// in (це вхідні дані, які приходять у шейдер ззовні)
	// vec3 (координати у форматі вектора 3-х чисел типу float для кожної точки)
	// layout (location = 0) номер порту, який зв'язує C++ масив із цим шейдером
	layout(location = 0) in vec3 pos;  
	uniform float mirrorX; // Створюємо Uniform-змінну для керування віддзеркалення по оси X
	uniform float mirrorY; // Створюємо Uniform-змінну для керування віддзеркалення по оси Y
	void main()	{ // Головна функція шейдера, яка викликається окремо для кожної вершини
			gl_Position = vec4(pos.x * mirrorX, pos.y * mirrorY, pos.z, 1.0); // Змінна, в яку записується фінальна позиція точки на екрані, вона вимагає 3 координати (X, Y, Z) та 4 - перспективу 
	}
)";

// Створюємо текст фрагметного шейдера, який буде керувати кольором пікселей
static const char* fShader = R"(
	#version 330
	out vec4 colour; // out (вихідні дані, які шейдер відає назовні для подальшого відображення пікселю на екран
	uniform vec4 objectColor; // Створюємо Uniform-змінну для передачі кольору між C++ та відеокартою
		void main()	{ // Головна функція шейдера, яка викликається окремо для кожного пікселя
			colour = objectColor; // Керуємо коліром в форматі RGBA, присвоюємо пікселю той колір, який передасть C++
		}
)";

// Функція створення фігури
Figure CreateFigure(GLfloat* vertices, int size) {
	Figure newFig;
	newFig.verteciesAmount = size / (3 * sizeof(GLfloat)); // Рахуємо кількість вершин
	glGenVertexArrays(1, &newFig.VAO); // Створюємо VAO, яка буде запам'ятовувати всі налаштування буферів 
	glBindVertexArray(newFig.VAO); // Активуємо VAO
	glGenBuffers(1, &newFig.VBO); // Створюємо VBO буфер, куди фізично завантажимо координати фігури
	glBindBuffer(GL_ARRAY_BUFFER, newFig.VBO); // Активуємо VBO
	glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW); // Копіюємо масив точок з ОЗП у VRAM
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0); // Створюємо декодер-пояснення як читати байти (слот №0, 3 числа, типу флоат, без нормалізації, крок 0, зміщення 0)
	glEnableVertexAttribArray(0); // Вмикаємо декодер для слоту 0
	glBindBuffer(GL_ARRAY_BUFFER, 0); // Відв'язуємо буфери, щоб випадково не стерти дані в іншому місці програми
	glBindVertexArray(0);
	return newFig;
}

// Допоміжна функція для компіляції одного шейдера
void AddShader(GLuint theProgram, const char* shaderCode, GLenum shaderType)
{
	GLuint theShader = glCreateShader(shaderType); // Створюємо порожній об'єкт шейдера потрібного типу (вершинний або фрагментний)
	const GLchar* theCode[1]; 
	theCode[0] = shaderCode; // Передаємо текст програми нашого шейдера
	GLint codeLength[1];
	codeLength[0] = strlen(shaderCode); // Рахуємо довжину тексту
	glShaderSource(theShader, 1, theCode, codeLength); // Передаємо текст шейдера у створений порожній об'єкт шейдера
	glCompileShader(theShader); // Компілюємо шейдер
	// Перевіряємо на помилки в коді GLSL шейдера
	GLint result = 0;
	GLchar eLog[1024] = { 0 };
	glGetShaderiv(theShader, GL_COMPILE_STATUS, &result);
	if (!result)
	{
		glGetShaderInfoLog(theShader, 1024, NULL, eLog);
		fprintf(stderr, "Error compiling the %d shader:	'%s'\n", shaderType, eLog); // Якщо компіляція провалилася до виводимо інформацію
		return;
	}
	glAttachShader(theProgram, theShader); // Інакше прикріплюємо цей скомпільований шейдер до нашої програми
}

void CompileShaders() {
	shader = glCreateProgram(); // Створюємо програму-інструкцію для відеокарти
	if (!shader)
	{
		printf("Failed to create shader\n");
		return;
	}
	AddShader(shader, vShader, GL_VERTEX_SHADER); // Додаємо та компілюємо вершинний шейдер
	AddShader(shader, fShader, GL_FRAGMENT_SHADER); // Додаємо та компілюємо фрагментний шейдер
	GLint result = 0;
	GLchar eLog[1024] = { 0 };
	glLinkProgram(shader); // З'єднує шейдер в одну інструкцію
	// Перевіряємо чи успішно вони з'єдналися між собою
	glGetProgramiv(shader, GL_LINK_STATUS, &result);
	if (!result)
	{
		glGetProgramInfoLog(shader, sizeof(eLog), NULL, eLog);
		printf("Error linking program: '%s'\n", eLog);
		return;
	}
	// Перевіряємо, чи зможе взагалі цей шейдер запуститися на поточній відеокарті
	glValidateProgram(shader);
	glGetProgramiv(shader, GL_VALIDATE_STATUS, &result);
	if (!result)
	{
		glGetProgramInfoLog(shader, sizeof(eLog), NULL, eLog);
		printf("Error validating program: '%s'\n", eLog);
		return;
	}
}

int main() {
	if (!glfwInit()) // Спроба ініціалізації бібліотеки GLFW, яка потрібна для створення віконих інтерфейсів та обробки подій в графічних програмах
	{
		printf("GLFW failed to start :(");
		glfwTerminate(); // У разі не ініціалізації бібліотеки GLFW виконується очищення ресурсів і завершення виконання програми
		return 1;
	}
	// Налаштовуємо OpenGL перед створенням вікна
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // Встановлюємо головну версію OpenGL 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); // Встановлюємо додаткову версію OpenGL 
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // Використовуємо Core-профіль, профіль сучасного OpenGL без застарілих функцій
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Вмикаємо зворотну сумісність для коректної роботи з macOS та новими драйверами
	GLFWwindow* mainWindow = glfwCreateWindow(WIDTH, HEIGHT, "by Vladislav Samoilov KNT-223", NULL, NULL); // Створюємо головне вікно програми із заданими розмірами та заколовком
	if (!mainWindow) // Перевіряємо ініціалізацію вікна
	{
		printf("GLFW window creation failed :(");
		glfwTerminate(); // Закриваємо GLFW, якщо вікно не вдалося створити та завершаємо виконання програми
		return 1;
	}
	int bufferWidth, bufferHeight; // Змінні для збереження реального розміру області рендерингу (в пікселях)
	glfwGetFramebufferSize(mainWindow, &bufferWidth, &bufferHeight); // Отримуємо фактичний розмір буфера кадру вікна
	glfwMakeContextCurrent(mainWindow); // Робимо контекст OpenGL створеного вікна поточним для цього потоку програми
	glewExperimental = GL_TRUE; // Дозволяємо GLEW завантажувати експериментальні та нові розширення OpenGL
	if (glewInit() != GLEW_OK) // Перевіряємо ініціалізацію бібліотеки GLEW, який динамічно завантажує функції OpenGL для відеокарти
	{
		printf("GLEW failed to start >_<");
		glfwDestroyWindow(mainWindow); // Знищуємо створене вікно
		glfwTerminate(); // Завершуємо роботу GLFW
		return 1;
	}
	glViewport(0, 0, bufferWidth, bufferHeight); // Налаштовуємо область перегляду OpenGL
	// Координати вершин прямокутника
	GLfloat squareV[] = {
		-0.5f, -0.4f, 0.0f, // Лівий нижній кут
		-0.5f,  0.4f, 0.0f, // Лівий верхній кут
		0.5f, -0.4f, 0.0f, // Правий нижній кут
		-0.5f,  0.4f, 0.0f, // Лівий верхній кут 
		0.5f,  0.4f, 0.0f, // Правий верхній кут
		0.5f, -0.4f, 0.0f  // Правий нижній кут
	};
	// Координати вершин трикутника
	GLfloat tringleV[] = {
		-0.5f, 0.4f, 0.0f,
		0.0f, 0.8f, 0.0f,
		0.5f, 0.4f, 0.0f
	};
	// Координати вершин трикутника
	GLfloat tringleLeftV[] = {
		-0.5f, -0.4f, 0.0f,
		-0.5f, 0.4f, 0.0f,
		-0.2f, 0.0f, 0.0f
	};

	// Координати вершин трикутника
	GLfloat tringlesV[] = {
		-0.5f, -0.4f, 0.0f,
		-0.2f, 0.0f, 0.0f,
		0.0f, -0.4f, 0.0f,

		0.5f, -0.4f, 0.0f,
		0.2f, 0.0f, 0.0f,
		0.0f, -0.4f, 0.0f,

		0.2f, 0.0f, 0.0f,
		-0.2f, 0.0f, 0.0f,
		0.0f, -0.4f, 0.0f,
	};

	Figure squre = CreateFigure(squareV, sizeof(squareV)); // Створюємо геометрію прямокутника 
	Figure tringle = CreateFigure(tringleV, sizeof(tringleV)); // Створюємо геометрію трикутника
	Figure tringleLeft = CreateFigure(tringleLeftV, sizeof(tringleLeftV)); // Створюємо геометрію трикутника
	Figure tringles = CreateFigure(tringlesV, sizeof(tringlesV)); // Створюємо геометрію трикутника
	CompileShaders(); // Компілюємо шейдери
	while (!glfwWindowShouldClose(mainWindow)) // Головний цикл роботи програми, працює, поки користувач не закриє вікно
	{
		glfwPollEvents(); // Перевіряємо та обробляємо події
		glClearColor(0.0f, 0.3f, 1.0f, 1.0f); // Встановлюємо колір для очищення екрану RGBA
		glClear(GL_COLOR_BUFFER_BIT); // Очищаємо буфер кольору кадру за допомогою вказаного вище кольору
		glUseProgram(shader); // Вмикаємо шейдерну програму
		GLint colorLoc = glGetUniformLocation(shader, "objectColor"); // Знаходимо змінну "objectColor" у пам'яті шейдера
		GLint mirrorX = glGetUniformLocation(shader, "mirrorX"); // Знаходимо змінну "mirrorX" у пам'яті шейдера
		GLint mirrorY = glGetUniformLocation(shader, "mirrorY"); // Знаходимо змінну "mirrorY" у пам'яті шейдера
		glUniform4f(colorLoc, 1.0f, 0.4, 0.12f, 1.0f); // Передаємо значення коліру до шейдеру
		glUniform1f(mirrorX, 1.0f); // Передаємо значення віддзеркалення по осі х до шейдеру
		glUniform1f(mirrorY, 1.0f); // Передаємо значення віддзеркалення по осі у до шейдеру

		glBindVertexArray(squre.VAO); // Вмикаємо налаштування фігури
		glDrawArrays(GL_TRIANGLES, 0, squre.verteciesAmount); // Даємо команду намалювати фігуру прямокутник

		glBindVertexArray(tringle.VAO); // Вмикаємо налаштування фігури
		glDrawArrays(GL_TRIANGLES, 0, tringle.verteciesAmount); // Даємо команду намалювати фігуру трикутник

		glBindVertexArray(tringleLeft.VAO); // Вмикаємо налаштування фігури
		glUniform4f(colorLoc, 1.0f, 0.5f, 0.0f, 1.0f); // Перемикаємо на інший колір
		glDrawArrays(GL_TRIANGLES, 0, tringleLeft.verteciesAmount); // Даємо команду намалювати фігуру
		glUniform1f(mirrorX, -1.0f); // Передаємо значення віддзеркалення по осі х до шейдеру
		glDrawArrays(GL_TRIANGLES, 0, tringleLeft.verteciesAmount); // Даємо команду намалювати фігуру

		glBindVertexArray(tringles.VAO); // Вмикаємо налаштування фігури
		glDrawArrays(GL_TRIANGLES, 0, tringles.verteciesAmount); // Даємо команду намалювати фігуру

		// Очищення, для того, щоб інші об'єкти не малювалися цим шейдером
		glBindVertexArray(0);
		glUseProgram(0);
		glfwSwapBuffers(mainWindow); // Міняємо місцями задній буфер (де ми малювали) і передній буфер (який бачить користувач)
	}
	return 0;
}