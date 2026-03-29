/*
 * Alumno : Jose pablo Perez De Anda
 * Materia: Graficas Por Computadora
 * Profesor: Cachalui
 *
 * Proyecto : proyecto_caja (Caja Enigmática estilo The Room)
 */

/*
 * Control de versiones:
 * 1.- Establecimos el esqueleto inicial del proyecto de graficas por computadora
 * 2.- Agregamos el sistema de cámara con rotación polar
 * 3.- Creamos el plano base (piso) con normales
 */

// ============================================================================
// INCLUDES
// ============================================================================
#include <math.h>
#include <stdio.h>    // ← necesario para fopen/fread (cargar BMPs)
#include <stdlib.h>   // ← necesario para malloc/free
#include <GL/glut.h>

// ============================================================================
// VARIABLES GLOBALES
// ============================================================================

/*
 * IDs de texturas del Skybox:
 * OpenGL maneja las texturas con números enteros (IDs)
 * Necesitamos 6 IDs, uno por cada cara del cubo del skybox
 *
 * skyboxTexturas[0] = derecha  (+X)
 * skyboxTexturas[1] = izquierda(-X)
 * skyboxTexturas[2] = arriba   (+Y)
 * skyboxTexturas[3] = abajo    (-Y)
 * skyboxTexturas[4] = frente   (-Z)
 * skyboxTexturas[5] = atrás    (+Z)
 */
GLuint skyboxTexturas[6];

// Tamaño del cubo del skybox (debe ser mayor que camDist máxima)
float skyboxTamanio = 200.0f;

//textura para la caja
GLuint texturaMaderaCaja = 0; //ID de la textura de la caja (cargada solo una vez)
GLuint texturaTapaMadera = 0 ; //ID de la textura de la tapa




// ------ CÁMARA ------
float camDist = 25.0f;      // Distancia de la cámara al origen
float camAngleX = 0.0f;     // Ángulo horizontal (rotación izquierda-derecha)
float camAngleY = 20.0f;    // Ángulo vertical (rotación arriba-abajo)

/*
 * Variables para el MOUSE:
 * Necesitamos recordar DÓNDE estaba el mouse en el frame anterior
 * para calcular cuánto se movió (el "delta")
 *
 * lastMouseX, lastMouseY → posición del mouse en el último frame
 * mousePresionado        → ¿está el botón izquierdo apretado ahora mismo?
 * mouseDeltaX/Y          → cuántos píxeles se movió desde el último frame
 */
int  lastMouseX     = 0;
int  lastMouseY     = 0;
bool mousePresionado = false;





// ------ LUZ PRINCIPAL ------
/*
 * lightPos[4] = { x, y, z, w }
 * w = 1.0 → luz puntual (tiene posición)
 * w = 0.0 → luz direccional (como el sol)
 */
float lightPos[4] = { 5.0f, 10.0f, 5.0f, 1.0f };
/*
 * Variables para mover el foco con teclado:
 * El foco se mueve en los 3 ejes para poder iluminar
 * diferentes partes de la caja
 */
float focoX = 5.0f;   // Posición horizontal
float focoY = 10.0f;  // Posición vertical (altura)
float focoZ = 5.0f;   // Posición de profundidad


// ------ ANIMACIONES Y ARTICULACIONES ------
//float tapRotation = 0.0f;        // Ángulo de apertura de tapa
//float ruedaRotation = 0.0f;      // Ángulo de rueda giratoria
//float brazoAngulo1 = 0.0f;       // Primer joint del brazo (articulación 1)
//float brazoAngulo2 = 0.0f;       // Segundo joint del brazo (articulación 2)

// ------ ESTADO DE SELECCIÓN ------
int selectedPart = -1;           // -1=ninguno, 0=tapa, 1=rueda, 2=brazo
float animSpeedMultiplier = 1.0f;// 1.0=velocidad normal, 2.0+=acelerada (si está seleccionada)

// ------ FLAGS DE VISUALIZACIÓN (Controles con teclas) ------
bool bCull = false;              // F1: Culling (quitar caras ocultas)
bool bDepth = true;              // F2: Depth test (prueba de profundidad)
bool bWireframe = false;         // F3: Wireframe (modo alambre)
bool bSmooth = false;            // F4: Smooth shading (sombreado suave vs plano)


// ============================================================================
// VARIABLES PARA CAJA, TAPA Y GEMA
// ============================================================================

// ------ DIMENSIONES DE LA CAJA ------
float cajaAncho = 6.0f;          // Ancho en X
float cajaProfundo = 8.0f;       // Profundidad en Z
float cajaAlto = 5.0f;           // Alto en Y

// ------ TAPA (BISAGRA LATERAL) ------
float tapRotation = 0.0f;        // Ángulo actual de rotación
float tapMaxRotation = 120.0f;   // Ángulo máximo que puede girar
float tapRotationSpeed = 1.5f;   // Grados por frame
bool tapIsOpening = false;       // ¿Está en proceso de abrir?

// ------ GEMA ADENTRO ------
float gemaRadius = 1.2f;         // Radio de la esfera de la gema
float gemaY = 1.5f;              // Altura a la que flota dentro de la caja
float alturaLuz = 4.0f;          // Altura de la luz (para iluminación)






// ============================================================================
// PROTOTIPOS DE FUNCIÓN (Forward declarations) para mas comodidad
// ============================================================================
void cargarTexturasSkybox();

void dibujarSkybox();
void dibujarFoco();
void dibujarPiso();
void dibujarCaja();
void dibujarTapa();
void display();
void myReshape(int w, int h);
void configurarOpenGl();

void teclado(unsigned char key, int x, int y);
void tecladoEspecial(int key, int x, int y);
void mouseBoton(int boton, int estado, int x, int y);
void mouseMovimiento(int x, int y);



// ============================================================================
// FUNCIÓN: dibujarFoco()
//
// Dibuja una esfera pequeña amarilla en la posición de la luz.
// Esto nos permite VER dónde está la fuente de luz en el espacio.
//
// TRUCO IMPORTANTE:
// Desactivamos GL_LIGHTING antes de dibujar el foco y lo reactivamos después.
//
// ¿Por qué? Porque si la iluminación está activa, OpenGL intentaría
// iluminar al foco con su propia luz — lo cual causaría que se vea
// oscuro en algunos ángulos (absurdo para una fuente de luz).
// Al desactivarla, el foco se pinta con su color puro siempre: amarillo brillante.
// ============================================================================
void dibujarFoco() {
    glPushMatrix();

        // Nos movemos a la posición de la luz
        glTranslatef(focoX, focoY, focoZ);

        // Desactivamos iluminación → el foco se pinta con color puro
        glDisable(GL_LIGHTING);

            /*
             * Color amarillo puro para que parezca que emite luz
             * R=1.0, G=1.0, B=0.0 → amarillo
             */
            glColor3f(1.0f, 1.0f, 0.0f);

            /*
             * glutSolidSphere(radio, slices, stacks):
             * Dibuja una esfera sólida
             *
             * - radio:  tamaño de la esfera (0.3 = pequeña, discreta)
             * - slices: "rebanadas" verticales (como meridianos del globo)
             *           más = más redonda, más costosa
             * - stacks: "capas" horizontales (como paralelos del globo)
             *           más = más redonda, más costosa
             *
             * Con 12,12 se ve suficientemente redonda sin ser costosa
             */
            glutSolidSphere(0.3, 12, 12);

        // Reactivamos iluminación para todo lo que venga después
        glEnable(GL_LIGHTING);

    glPopMatrix();
}




// ============================================================================
// FUNCIÓN: display()
// Función que se llama cada vez que GLUT necesita redibujar la pantalla
// ============================================================================
void display() {
    // 1. LIMPIEZA DE BUFFERS
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glLoadIdentity(); // Reiniciamos la matriz de transformaciones al estado inicial
   
     /*
     * Aplicamos los FLAGS de visualización ANTES de dibujar
     * para que afecten todo lo que se dibuja en este frame
     */
    if (bCull)      glEnable(GL_CULL_FACE);
    else glDisable(GL_CULL_FACE);
    if (bDepth)     glEnable(GL_DEPTH_TEST);             
    else glDisable(GL_DEPTH_TEST);
    if (bWireframe) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (bSmooth)    glShadeModel(GL_SMOOTH); 
    else glShadeModel(GL_FLAT);




    // Aquí irá la jerarquía de objetos (caja, tapa, mecanismos, etc.) en el futuro

    // ========================================================================
    // 2. CÁMARA: Posicionamiento del observador (PUNTO 3 de la rúbrica)
    // ========================================================================
    
    /* 
     * La cámara se posiciona usando COORDENADAS ESFÉRICAS:
     * - camDist: radio (distancia del centro)
     * - camAngleX: azimut (giro horizontal)
     * - camAngleY: elevación (giro vertical)
     * 
     * Convertimos de GRADOS a RADIANES (porque sin/cos usan radianes):
     * 1 grado = 0.01745 radianes
     */
    float rX = camAngleX * 0.01745f;  // Conversión a radianes (eje X)
    float rY = camAngleY * 0.01745f;  // Conversión a radianes (eje Y)

    /*
     * Cálculo de posición del "ojo" (cámara) usando TRIGONOMETRÍA ESFÉRICA:
     * - eyeX: movimiento en el plano XZ horizontal
     * - eyeY: movimiento vertical (arriba-abajo)
     * - eyeZ: profundidad (entrada-salida)
     */
    float eyeX = camDist * sin(rX) * cos(rY);
    float eyeY = camDist * sin(rY);
    float eyeZ = camDist * cos(rX) * cos(rY);

    /*
     * gluLookAt(eyeX, eyeY, eyeZ, centerX, centerY, centerZ, upX, upY, upZ):
     * - Primer triplete (eye): posición de la cámara en el espacio
     * - Segundo triplete (center): punto hacia donde mira la cámara
     * - Tercer triplete (up): vector que indica qué dirección es "arriba"
     *   (0, 1, 0) significa que el eje Y es hacia arriba
     */
    gluLookAt(eyeX, eyeY, eyeZ,      // Posición del ojo (cámara)
              0.0f, 0.0f, 0.0f,      // Mira hacia el origen (centro del mundo)
              0.0f, 1.0f, 0.0f);     // Vector "arriba" (eje Y positivo)
				     
    //**Actualizamos la posicion de la luz cada frame**//
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

     
	// ========================================================================
	// 2.5 ANIMACIÓN DE LA TAPA
	// ========================================================================
	/*
	 * Si la tapa está abriendo, incrementamos su ángulo cada frame.
	 * Si llegó al máximo, la detenemos.
	 */
	if (tapIsOpening) {
    		if (tapRotation < tapMaxRotation) {
        		tapRotation += tapRotationSpeed;
		        glutPostRedisplay(); // <- EL MOTOR: Pide dibujar el SIGUIENTE cuadro
    		}
	} else {
    		if (tapRotation > 0.0f) {
        		tapRotation -= tapRotationSpeed;
        		glutPostRedisplay(); // <- EL MOTOR: Pide dibujar el SIGUIENTE cuadro
    		}
	}

	



    // ========================================================================
    // 3. RENDERIZADO DEL ESCENARIO
    // ========================================================================
    // El orden de dibujado importara
    /*
     * ORDEN IMPORTANTÍSIMO:
     * 1° Skybox  → fondo, sin escribir Z-buffer
     * 2° Foco    → sin iluminación (emisivo)
     * 3° Piso    → con iluminación
     * 4° Caja    → con iluminación
     * 5 tapaCaja -> con iluminacion
     */
    dibujarSkybox(); //dibujamos el entorno primero
    dibujarFoco(); //dibuja el foco
    dibujarPiso(); // Dibuja el plano base
    dibujarCaja(); //dibujamos la caja :D
    dibujarTapa(); // dibujamos la tapita de la caja

    // ========================================================================
    // 4. INTERCAMBIO DE BUFFERS
    // ========================================================================
    glutSwapBuffers(); // Muestra la imagen renderizada en pantalla
}

// ============================================================================
// FUNCIÓN: dibujarPiso()
// Dibuja un plano rectangular horizontal que servirá como base del escenario
// ============================================================================
void dibujarPiso() {
    // Color del piso: gris medio
    glColor3f(0.4f, 0.4f, 0.4f);

    /*
     * GL_QUADS: Modo de dibujo para cuadriláteros
     * Cada 4 vértices consecutivos forman UN CUADRILÁTERO (4 lados)
     * 
     * Otros modos comunes:
     * - GL_TRIANGLES: cada 3 vértices = 1 triángulo
     * - GL_LINES: cada 2 vértices = 1 línea
     * - GL_POINTS: cada vértice = 1 punto
     * - GL_QUADS: cada 4 vértices = 1 cuadrilátero (lo que usamos)
     */
    glBegin(GL_QUADS);

        /*
         * glNormal3f(x, y, z): Define el vector normal de la superficie
         * Un vector normal es perpendicular a la superficie y es CRUCIAL para la iluminación
         * 
         * Para nuestro piso HORIZONTAL (en el plano XZ con Y=0):
         * - La cara superior apunta HACIA ARRIBA → normal = (0, 1, 0)
         * - Si fuera vertical, sería diferente
         * 
         * La luz usa esta normal para calcular cuánta luz rebota en la superficie
         */
        glNormal3f(0.0f, 1.0f, 0.0f);

        /*
         * Los 4 vértices definen un RECTÁNGULO GIGANTE sobre el plano XZ (donde Y=0.0)
         * 
         * Sistema de coordenadas:
         * - Eje X: horizontal (izquierda-derecha)
         * - Eje Y: vertical (arriba-abajo)
         * - Eje Z: profundidad (adentro-afuera de pantalla)
         * 
         * Observa que todos tienen Y=0.0 (están al nivel del suelo)
         */
        glVertex3f(-30.0f, 0.0f, -30.0f); // Esquina posterior izquierda
        glVertex3f(-30.0f, 0.0f,  30.0f); // Esquina frontal izquierda
        glVertex3f( 30.0f, 0.0f,  30.0f); // Esquina frontal derecha
        glVertex3f( 30.0f, 0.0f, -30.0f); // Esquina posterior derecha

    glEnd(); // Fin del cuadrilátero
}

// ============================================================================
// FUNCIÓN: myReshape(int w, int h)
// Se llama automáticamente cada vez que GLUT cambia el tamaño de la ventana
// 
// PROPÓSITO: Ajustar la PROYECCIÓN (cómo se transforma 3D a 2D en pantalla)
// para que nunca se vea distorsionada, sin importar el tamaño de la ventana
// ============================================================================
void myReshape(int w, int h) {
    
    /*
     * glViewport(x, y, width, height):
     * Define el ÁREA DE DIBUJO de la ventana
     * - (0, 0): esquina inferior izquierda
     * - (w, h): nuevo ancho y alto de la ventana
     * 
     * Esto asegura que OpenGL use toda el área de la ventana
     */
    glViewport(0, 0, w, h);

    /*
     * glMatrixMode(GL_PROJECTION):
     * Cambia a la MATRIZ DE PROYECCIÓN
     * Esta matriz define cómo se transforman las coordenadas 3D del mundo
     * a las coordenadas 2D de la pantalla
     * 
     * Tipos de matrices en OpenGL:
     * - GL_PROJECTION: proyección (perspectiva, ortográfica)
     * - GL_MODELVIEW: modelo y vista (transformaciones de objetos y cámara)
     */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity(); // Reinicia la matriz de proyección

    /*
     * gluPerspective(fov, aspect, near, far):
     * Define una PROYECCIÓN EN PERSPECTIVA (como el ojo humano)
     * 
     * Parámetros:
     * - 60.0f: Ángulo de visión vertical en grados (field of view)
     *          Un valor mayor = zoom out (ves más), menor = zoom in (ves menos)
     * - (float)w/h: Relación de aspecto (ancho/alto)
     *               Si cambias tamaño ventana, esto evita distorsión
     *               CRÍTICO: (float) convierte divisiones a punto flotante
     * - 1.0f: Plano CERCANO (near plane) - distancia mínima que ves
     *         Objetos más cerca que esto NO se ven
     * - 300.0f: Plano LEJANO (far plane) - distancia máxima que ves
     *          Objetos más lejos que esto NO se ven (se quedan negros)
     */
    gluPerspective(60.0f, (float)w / h, 1.0f, 2000.0f);

    /*
     * glMatrixMode(GL_MODELVIEW):
     * Volvemos a la MATRIZ DE MODELO-VISTA
     * Ahora podemos volver a dibujar objetos y aplicarles transformaciones
     */
    glMatrixMode(GL_MODELVIEW);
}


/*
 * Funcion Dibujar caja
 * Dibuja una cara rectangular abierta por arriba (sin tapita)
 * Las dimensiones son : cajaAncho(x), cajaAlto(y) , cajaProfundo(Z)
 * */
void dibujarCaja(){

	/*
	 * nota, en esta seccion para dibujar la caja es obvio que debemos 
	 * de limitar cada cara mediante planos 
	 * glBegin(GL_QUADS) le dice a opengl "voy a mandarte vertices" y quiero
	 * 	que los juntes de esta forma "GL_QUADS -> cuadrilatero"
	 * En cada una de las caras agregamos la direccion a la normal 
	 * 	para que las usemos al momento de hacer reflexion
	 * 	es decir para la luz
	 * glNormal3f
	 *
	 * 	-> la divicion entre 2 se da porque la caja se esta dibujando desde el origen
	 * 	   cada cara se parte a la mitad :D
	 * */

	/*La caja esta cerrada en el origen 0,0,0
	 * usare push/pop para no afectar otras transformaciones
	 * */
	glPushMatrix();

	//area de textura de la caja :D
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texturaMaderaCaja);

	//trasladamos la cajita para que no este enterrada
	glTranslatef(0.0f , cajaAlto / 2.0f + 0.01f , 0.0f);

	//definimos el color de la caja: madera oscura cafe 
	glColor3f(0.4f,0.25f,0.1f);

    // ========== CARA 1: FRENTE (Z negativo) ==========
    /*
     * Normal apunta hacia AFUERA (hacia -Z)
     * Vértices en orden antihorario (CCW) visto desde afuera
     */
    glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, -1.0f);

	glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f);  // Arriba izquierda
									  
	glTexCoord2f(1.0f, 1.0f);
        glVertex3f( cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f);  // Arriba derecha
									  
	glTexCoord2f(1.0f, 0.0f);
        glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);  // Abajo derecha
									  
	glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);  // Abajo izquierda
    glEnd();
    // ========== CARA 2: ATRÁS (Z positivo) ==========
    glBegin(GL_QUADS);
        glNormal3f(0.0f, 0.0f, 1.0f);

	glTexCoord2f(0.0f, 1.0f);
        glVertex3f( cajaAncho/2.0f,  cajaAlto/2.0f, cajaProfundo/2.0f);   // Arriba derecha
        
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f, cajaProfundo/2.0f);   // Arriba izquierda
        glTexCoord2f(1.0f, 0.0f);
	glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, cajaProfundo/2.0f);   // Abajo izquierda
        
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f, cajaProfundo/2.0f);   // Abajo derecha
    glEnd();
    
    // ========== CARA 3: IZQUIERDA (X negativo) ==========
    glBegin(GL_QUADS);
        glNormal3f(-1.0f, 0.0f, 0.0f);

	glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f,  cajaProfundo/2.0f);  // Arriba atrás
        
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(-cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f); // Arriba frente

	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);  // Abajo frente

	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);  // Abajo atrás
    glEnd();
    
    // ========== CARA 4: DERECHA (X positivo) ==========
    glBegin(GL_QUADS);
        glNormal3f(1.0f, 0.0f, 0.0f);

	glTexCoord2f(0.0f, 1.0f);
        glVertex3f(cajaAncho/2.0f,  cajaAlto/2.0f, -cajaProfundo/2.0f);   // Arriba frente
									  
	glTexCoord2f(1.0f, 1.0f);								  
        glVertex3f(cajaAncho/2.0f,  cajaAlto/2.0f,  cajaProfundo/2.0f);   // Arriba atrás
									  
	glTexCoord2f(1.0f, 0.0f);
        glVertex3f(cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);   // Abajo atrás

	glTexCoord2f(0.0f, 0.0f);
        glVertex3f(cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);   // Abajo frente
    glEnd();
    
    // ========== CARA 5: FONDO (Y negativo) ==========
    glBegin(GL_QUADS);
        glNormal3f(0.0f, -1.0f, 0.0f);

	glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);  // Frente izquierda
									  
	glTexCoord2f(1.0f, 1.0f);
        glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f, -cajaProfundo/2.0f);  // Frente derecha

	glTexCoord2f(1.0f, 0.0f);
        glVertex3f( cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);  // Atrás derecha

	glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-cajaAncho/2.0f, -cajaAlto/2.0f,  cajaProfundo/2.0f);  // Atrás izquierda
    glEnd();
    

    glDisable(GL_TEXTURE_2D); //-> no entiendo por que es necesario esto
    glPopMatrix();
}


// ============================================================================
// FUNCIÓN: dibujarTapa()
//
// Dibuja la tapa de la caja como un rectángulo plano.
// Por ahora está FIJA (sin rotación).
// Más adelante le agregaremos rotación.
//
// GEOMETRÍA:
// - Está en la parte superior de la caja (Y = cajaAlto/2)
// - Tiene el mismo ancho que la caja (X: -cajaAncho/2 a +cajaAncho/2)
// - Tiene la misma profundidad que la caja (Z: -cajaProfundo/2 a +cajaProfundo/2)
// ============================================================================
void dibujarTapa() {
    
    glPushMatrix();

    //cargamos la textura de la tapita
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaTapaMadera);
    
    // La tapa está en la parte superior de la caja
    // Trasladamos a la altura de la parte superior
    glTranslatef(0.0f, cajaAlto + 0.01f, 0.0f);
   
    /*
     * BISAGRA LATERAL (lado derecho en X positivo):
     * 
     * La tapa gira hacia AFUERA (hacia +X positivo)
     * 
     * Pasos:
     * 1. Trasladamos al punto de la bisagra (X positivo)
     * 2. Rotamos alrededor del eje Y (giro horizontal)
     * 3. Trasladamos de vuelta
     */
    glTranslatef(cajaAncho/2.0f, 0.0f, 0.0f);   // Vamos a la bisagra derecha
    glRotatef(-tapRotation, 0.0f, 0.0f, 1.0f);   // Rotamos alrededor del eje Y
    glTranslatef(-cajaAncho/2.0f, 0.0f, 0.0f);  // Volvemos


    // Color: madera más clara que la caja (para diferenciarse)
    glColor3f(0.5f, 0.3f, 0.15f);
    

    // Dibujamos la tapa como un rectángulo plano
    glBegin(GL_QUADS);
        
        /*
         * Normal apunta hacia ARRIBA (Y positivo)
         * porque la tapa está mirando hacia el cielo
         */
        glNormal3f(0.0f, 1.0f, 0.0f);
        
        // Los 4 vértices de la tapa
	
	glTexCoord2f(0.0f, 1.0f);
        glVertex3f(-cajaAncho/2.0f, 0.0f, -cajaProfundo/2.0f);  // Frente izquierda

	glTexCoord2f(1.0f, 1.0f);
        glVertex3f( cajaAncho/2.0f, 0.0f, -cajaProfundo/2.0f);  // Frente derecha
								
	glTexCoord2f(1.0f, 0.0f);
        glVertex3f( cajaAncho/2.0f, 0.0f,  cajaProfundo/2.0f);  // Atrás derecha

	glTexCoord2f(0.0f, 0.0f);
        glVertex3f(-cajaAncho/2.0f, 0.0f,  cajaProfundo/2.0f);  // Atrás izquierda
        
    glEnd();
    
    glDisable(GL_TEXTURE_2D); //<- desconosco porque :D
    glPopMatrix();
}


// ============================================================================
// FUNCIÓN: cargarBMP()
//
// Carga una imagen BMP de 24 bits desde disco y la sube a la GPU como textura.
// Devuelve el ID de textura que OpenGL asigna.
//
// ¿Por qué BMP y no PNG/JPG?
// BMP es el formato más simple — sin compresión, datos crudos de píxeles.
// No necesitamos librerías externas para leerlo.
//
// Parámetros:
//   ruta → path al archivo .bmp (ej: "assets/1.bmp")
// ============================================================================
GLuint cargarBMP(const char* ruta) {

    // ------------------------------------------------------------------
    // 1. ABRIR EL ARCHIVO
    // ------------------------------------------------------------------
    FILE* archivo = fopen(ruta, "rb"); // "rb" = read binary (leer en binario)
    if (!archivo) {
        printf("ERROR: No se pudo abrir la textura: %s\n", ruta);
        return 0; // Devolvemos 0 = textura inválida
    }

    // ------------------------------------------------------------------
    // 2. LEER EL ENCABEZADO BMP
    // ------------------------------------------------------------------
    /*
     * Un archivo BMP tiene un encabezado de 54 bytes al inicio
     * que contiene información sobre el tamaño y formato de la imagen.
     *
     * Estructura del encabezado BMP:
     * Bytes  0- 1: "BM" (firma del formato)
     * Bytes  2- 5: Tamaño total del archivo
     * Bytes 10-13: Offset donde empiezan los datos de píxeles
     * Bytes 18-21: Ancho de la imagen en píxeles
     * Bytes 22-25: Alto de la imagen en píxeles
     */
    unsigned char encabezado[54];
    if (fread(encabezado, 1, 54, archivo) != 54) {
        printf("ERROR: Encabezado BMP inválido en: %s\n", ruta);
        fclose(archivo);
        return 0;
    }

    // Verificamos que sea un BMP válido (empieza con "BM")
    if (encabezado[0] != 'B' || encabezado[1] != 'M') {
        printf("ERROR: No es un archivo BMP válido: %s\n", ruta);
        fclose(archivo);
        return 0;
    }

    /*
     * Extraemos el ancho y alto del encabezado.
     * Los datos están en formato little-endian (byte menos significativo primero)
     * Por eso usamos este cálculo con corrimientos de bits (<<)
     *
     * Ejemplo para 4 bytes: byte[0] + byte[1]*256 + byte[2]*65536 + byte[3]*16777216
     */
    int ancho  = encabezado[18] | (encabezado[19] << 8) |
                 (encabezado[20] << 16) | (encabezado[21] << 24);
    int alto   = encabezado[22] | (encabezado[23] << 8) |
                 (encabezado[24] << 16) | (encabezado[25] << 24);

    // ------------------------------------------------------------------
    // 3. LEER LOS DATOS DE PÍXELES
    // ------------------------------------------------------------------
    /*
     * En BMP de 24 bits, cada píxel ocupa 3 bytes: Azul, Verde, Rojo (BGR)
     * NOTA: BMP guarda en orden BGR, pero OpenGL espera RGB
     * Lo corregimos más adelante con GL_BGR
     *
     * Tamaño total de datos = ancho × alto × 3 bytes por píxel
     */
    int tamanioDatos = ancho * alto * 3;
    unsigned char* datos = (unsigned char*)malloc(tamanioDatos);

    if (!datos) {
        printf("ERROR: Sin memoria para cargar: %s\n", ruta);
        fclose(archivo);
        return 0;
    }

    size_t bytesLeidos = fread(datos, 1, tamanioDatos, archivo);
    if(bytesLeidos !=(size_t)tamanioDatos){
	printf("ADVERTENCIA: lECTURA INCOMPLETA DE %s\n", ruta);
    }
    fclose(archivo);

    // ------------------------------------------------------------------
    // 4. CREAR LA TEXTURA EN OPENGL
    // ------------------------------------------------------------------

    GLuint idTextura;

    /*
     * glGenTextures(n, &ids):
     * Le pide a OpenGL que nos asigne 'n' IDs de textura libres
     * Los guarda en el array 'ids'
     * Es como reservar un "slot" en la memoria de la GPU
     */
    glGenTextures(1, &idTextura);

    /*
     * glBindTexture(tipo, id):
     * "Activa" esta textura para que las siguientes operaciones
     * de textura le afecten a ELLA y no a otra.
     * GL_TEXTURE_2D = textura bidimensional (la más común)
     */
    glBindTexture(GL_TEXTURE_2D, idTextura);

    /*
     * glTexParameteri: Configura cómo se comporta la textura
     *
     * GL_TEXTURE_MIN_FILTER: Filtro cuando la textura se ve MUY PEQUEÑA
     * GL_TEXTURE_MAG_FILTER: Filtro cuando la textura se ve MUY GRANDE
     * GL_LINEAR: Interpolación bilineal → se ve suave (no pixelada)
     *
     * GL_TEXTURE_WRAP_S/T: Qué hacer cuando las coordenadas salen de [0,1]
     * GL_CLAMP_TO_EDGE: Repite el píxel del borde (evita costuras en el skybox)
     */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    /*
     * glTexImage2D: Sube los datos de la imagen a la GPU
     *
     * Parámetros:
     * - GL_TEXTURE_2D: tipo de textura
     * - 0: nivel de mipmap (0 = imagen original)
     * - GL_RGB: formato INTERNO en la GPU (cómo almacenarlo)
     * - ancho, alto: dimensiones
     * - 0: borde (siempre 0 en OpenGL moderno)
     * - GL_BGR: formato de los datos QUE ESTAMOS ENVIANDO
     *           BMP guarda en BGR, por eso usamos GL_BGR y no GL_RGB
     * - GL_UNSIGNED_BYTE: tipo de dato (cada canal es 1 byte 0-255)
     * - datos: puntero a los píxeles
     */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ancho, alto,
                 0, GL_BGR, GL_UNSIGNED_BYTE, datos);

    free(datos); // Liberamos la memoria RAM (ya está en la GPU)

    printf("Textura cargada: %s (%dx%d)\n", ruta, ancho, alto);
    return idTextura;
}






// ============================================================================
// FUNCIÓN: configurarOpenGL()
//
// Se llama UNA SOLA VEZ al inicio del programa, antes de glutMainLoop().
// Establece el "estado global" de OpenGL es como
// "encender y calibrar el motor gráfico" antes de empezar a dibujar.
//
// Sin esta función, todo se ve plano y naranja porque OpenGL no sabe
// que existe una luz ni cómo calcular sombras y brillos.
// ============================================================================
void configurarOpenGL() {

	// Cargamos las texturas del skybox UNA SOLA VEZ
    	cargarTexturasSkybox();

	//cargamos la textura de la caja
	texturaMaderaCaja = cargarBMP("assets/textures/madera/Pino.bmp");
	//cargamos la textura de la tapa de madera
	texturaTapaMadera = cargarBMP("assets/textures/madera/Wood_Tile.bmp");	

    /*
    * GL_LIGHT_MODEL_TWO_SIDE:
    * Por defecto OpenGL solo ilumina la cara FRONTAL de cada polígono
    * Con esto activado, ilumina AMBAS caras (frente y reverso)
    * Perfecto para ver el interior de la caja iluminado
    */
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);	

    // ------------------------------------------------------------------
    // 1. COMPONENTES DE LA LUZ
    // ------------------------------------------------------------------

    /*
     * Luz AMBIENTAL: iluminación base que viene de todas partes
     * Simula la luz que rebota en el ambiente (como luz de día indirecta)
     * Sin esto, las caras que no ven la luz quedan completamente negras
     *
     * { R, G, B, Alpha } → valores entre 0.0 (oscuro) y 1.0 (brillante)
     * 0.3 = luz tenue pero suficiente para ver todo
     */
    GLfloat luzAmbiental[] = { 0.5f, 0.5f, 0.5f, 1.0f };

    /*
     * Luz DIFUSA: la luz directa que viene de la posición de la fuente
     * Es la que crea la diferencia entre caras iluminadas y en sombra
     * 0.7 = bastante fuerte, da buen contraste de profundidad
     */
    GLfloat luzDifusa[] = { 0.85f, 0.85f, 0.85f, 1.0f };

    // Aplicamos ambas componentes a GL_LIGHT0 (la primera luz de OpenGL)
    glLightfv(GL_LIGHT0, GL_AMBIENT,  luzAmbiental);
    glLightfv(GL_LIGHT0, GL_DIFFUSE,  luzDifusa);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // ------------------------------------------------------------------
    // 2. MATERIALES DE COLOR
    // ------------------------------------------------------------------

    /*
     * Sin esto, glColor3f() es IGNORADO cuando la iluminación está activa.
     * OpenGL usaría un material gris por defecto para todo.
     *
     * glEnable(GL_COLOR_MATERIAL) → "usa el color como material"
     * glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE) →
     *     "el color afecta tanto la luz ambiental como la difusa"
     *     "solo en las caras de frente (GL_FRONT)"
     *
     * Con esto, glColor3f(0.4, 0.25, 0.1) se ve como madera café real
     * en lugar del naranja plano de antes
     */
    glEnable(GL_COLOR_MATERIAL);
    /*
	 * GL_FRONT_AND_BACK → el color glColor3f() se aplica a AMBAS caras
 	* Esto es necesario porque activamos GL_LIGHT_MODEL_TWO_SIDE
	 * Si solo aplicamos color al frente, el reverso usa material blanco por defecto
    */
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);


    // ------------------------------------------------------------------
    // 3. ENCENDER LA LUZ
    // ------------------------------------------------------------------

    /*
     * OpenGL tiene hasta 8 luces (GL_LIGHT0 ... GL_LIGHT7)
     * Primero encendemos el "interruptor maestro" GL_LIGHTING,
     * luego encendemos la luz específica GL_LIGHT0
     */
    glEnable(GL_LIGHTING); // Interruptor maestro: activa el sistema de luces
    glEnable(GL_LIGHT0);   // Enciende la primera luz (la que configuramos arriba)

    // ------------------------------------------------------------------
    // 4. DEPTH TEST (Prueba de profundidad / Z-buffer)
    // ------------------------------------------------------------------

    /*
     * Sin esto, objetos lejanos se dibujan ENCIMA de objetos cercanos
     * El Z-buffer recuerda qué tan lejos está cada píxel
     * y descarta los píxeles que quedan detrás de otro objeto
     */
    glEnable(GL_DEPTH_TEST);

    // ------------------------------------------------------------------
    // 5. ORDEN DE VÉRTICES (Winding)
    // ------------------------------------------------------------------

    /*
     * GL_CCW = Counter-ClockWise (sentido antihorario)
     * Le dice a OpenGL que nuestras caras están definidas en ese orden
     * Esto es importante para que las normales apunten hacia afuera
     * y la iluminación funcione correctamente
     */
    glFrontFace(GL_CCW);
}


// ============================================================================
// FUNCIÓN: teclado()
//
// GLUT la llama automáticamente cada vez que el usuario presiona
// una tecla NORMAL (letras, números, espacio, escape, etc.)
//
// Parámetros:
//   key → el carácter de la tecla presionada (ej: 'w', 'a', 27=Escape)
//   x,y → posición del mouse cuando se presionó la tecla (raramente útil)
//
// IMPORTANTE: glutPostRedisplay() al final le dice a GLUT
// "algo cambió, por favor redibuja la pantalla"
// Sin esto, los cambios no se verían hasta que algo más forzara el redibujado
// ============================================================================
void teclado(unsigned char key, int x, int y) {
    switch (key) {

        // ------ ZOOM ------
        /*
         * 'w' acerca la cámara reduciendo camDist
         * 's' la aleja aumentando camDist
         * El límite de 5.0f evita que la cámara entre dentro de la caja
         */
        case 'w': case 'W':
            if (camDist > 5.0f) camDist -= 1.0f;
            break;

        case 's': case 'S':
            camDist += 1.0f;
            break;

        // ------ CONTROLES DE VISUALIZACIÓN (requisito 10) ------

        /*
         * CULLING (F1 en tu versión, aquí usamos 'c'):
         * Elimina las caras que NO están mirando hacia la cámara
         * Útil para ver el interior de objetos y mejorar rendimiento
         * Con la caja abierta, esto puede mostrar el interior sin la cara trasera
         */
        case 'c': case 'C':
            bCull = !bCull; // Alternamos: si estaba activo lo apagamos y viceversa
            break;

        /*
         * DEPTH TEST (F2 en tu versión, aquí usamos 'd'):
         * Si lo apagas, los objetos lejanos se dibujan ENCIMA de los cercanos
         * Útil para ver cómo funciona el Z-buffer
         */
        case 'd': case 'D':
            bDepth = !bDepth;
            break;

        /*
         * WIREFRAME (F3 en tu versión, aquí usamos 'z'):
         * Muestra solo los bordes de los polígonos sin rellenar
         * Muy útil para debuggear la geometría
         */
        case 'z': case 'Z':
            bWireframe = !bWireframe;
            break;

        /*
         * SMOOTH SHADING (F4 en tu versión, aquí usamos 'x'):
         * GL_SMOOTH → interpola colores entre vértices (degradado suave)
         * GL_FLAT   → cada polígono tiene UN solo color (bloques planos)
         */
        case 'x': case 'X':
            bSmooth = !bSmooth;
            break;
	
	// ------ MOVER EL FOCO CON TECLADO NUMÉRICO ------
        /*
         * Controles del teclado numérico para mover la luz:
         * 8 → foco hacia adentro  (Z negativo)
         * 2 → foco hacia afuera   (Z positivo)
         * 6 → foco hacia derecha  (X positivo)
         * 4 → foco hacia izquierda(X negativo)
         * 1 → foco sube           (Y positivo)
         * 0 → foco baja           (Y negativo)
         */
        case '8': focoZ -= 0.5f; break;
        case '2': focoZ += 0.5f; break;
        case '6': focoX += 0.5f; break;
        case '4': focoX -= 0.5f; break;
        case '1': focoY += 0.5f; break;
        case '0': if (focoY > 0.5f) focoY -= 0.5f; break;

	case ' ': //espacio
	    tapIsOpening = !tapIsOpening;
	    glutPostRedisplay();
	    break;
        /*
         * ESCAPE → cerrar el programa
         * El valor 27 es el código ASCII de la tecla Escape
         */
        case 27:
            exit(0);
            break;
    }

    /*
     * Actualizamos lightPos con las nuevas coordenadas del foco
     * para que OpenGL sepa desde dónde iluminar
     *
     * Es importante hacer esto AQUÍ (en el callback de teclado)
     * y no solo en display(), para que la posición siempre esté
     * sincronizada con la esfera visible
     */
    lightPos[0] = focoX;
    lightPos[1] = focoY;
    lightPos[2] = focoZ;
    // Le avisamos a GLUT que algo cambió y necesita redibujar
    glutPostRedisplay();
}


// ============================================================================
// FUNCIÓN: tecladoEspecial()
//
// GLUT la llama cuando el usuario presiona teclas ESPECIALES:
// flechas del teclado, F1-F12, Insert, Delete, etc.
// Las teclas especiales NO tienen un valor ASCII, por eso necesitan
// su propia función separada de teclado()
//
// Parámetros:
//   key → constante GLUT (ej: GLUT_KEY_UP, GLUT_KEY_F1)
//   x,y → posición del mouse al presionar
// ============================================================================
void tecladoEspecial(int key, int x, int y) {
    switch (key) {

        /*
         * Las flechas rotan la cámara manualmente
         * Es una alternativa al mouse para rotar la vista
         * Útil si alguien no quiere usar el mouse
         *
         * camAngleX → giro horizontal (izquierda/derecha)
         * camAngleY → giro vertical (arriba/abajo)
         *
         * El límite de camAngleY evita que la cámara se "voltee"
         * Si pasara de 89°, el vector "arriba" se invertiría y
         * la escena se vería al revés
         */
        case GLUT_KEY_LEFT:
            camAngleX -= 3.0f;
            break;

        case GLUT_KEY_RIGHT:
            camAngleX += 3.0f;
            break;

        case GLUT_KEY_UP:
            if (camAngleY < 89.0f) camAngleY += 3.0f;
            break;

        case GLUT_KEY_DOWN:
            if (camAngleY > 1.0f) camAngleY -= 3.0f;
            break;

        /*
         * F1-F4: controles alternativos de visualización
         * (los mismos que 'c','d','z','x' pero con teclas de función)
         */
        case GLUT_KEY_F1: bCull      = !bCull;      break;
        case GLUT_KEY_F2: bDepth     = !bDepth;     break;
        case GLUT_KEY_F3: bWireframe = !bWireframe; break;
        case GLUT_KEY_F4: bSmooth    = !bSmooth;    break;
    }


    glutPostRedisplay();
}


// ============================================================================
// FUNCIÓN: mouseBoton()
//
// GLUT la llama cuando el usuario PRESIONA o SUELTA un botón del mouse
//
// Parámetros:
//   boton  → qué botón: GLUT_LEFT_BUTTON, GLUT_RIGHT_BUTTON, GLUT_MIDDLE_BUTTON
//   estado → GLUT_DOWN (presionado) o GLUT_UP (soltado)
//   x, y   → posición del cursor en la ventana cuando ocurrió el evento
//             NOTA: en GLUT, Y=0 es la parte SUPERIOR de la ventana
//                   (al revés que OpenGL donde Y=0 es abajo)
// ============================================================================
void mouseBoton(int boton, int estado, int x, int y) {

    if (boton == GLUT_LEFT_BUTTON) {

        if (estado == GLUT_DOWN) {
            /*
             * El usuario ACABA DE PRESIONAR el botón izquierdo:
             * - Guardamos la posición actual del mouse como referencia
             * - Activamos la bandera de "mouse presionado"
             *
             * Esto es necesario porque mouseMovimiento() se llama
             * MUCHAS veces mientras el mouse se mueve, y necesita
             * saber desde dónde empezó el movimiento para calcular el delta
             */
            mousePresionado = true;
            lastMouseX = x;
            lastMouseY = y;

        } else { // GLUT_UP
            /*
             * El usuario SOLTÓ el botón:
             * Desactivamos la bandera para que mouseMovimiento()
             * no haga nada cuando el mouse se mueva sin estar presionado
             */
            mousePresionado = false;
        }
    }

    /*
     * SCROLL del mouse (rueda):
     * En GLUT, el scroll se reporta como botones 3 y 4
     * Botón 3 = scroll hacia arriba  → acercar cámara
     * Botón 4 = scroll hacia abajo   → alejar cámara
     * Solo nos importa el evento DOWN (ignoramos el UP del scroll)
     */
    if (boton == 3 && estado == GLUT_DOWN) {
        if (camDist > 5.0f) camDist -= 1.0f;
        glutPostRedisplay();
    }
    if (boton == 4 && estado == GLUT_DOWN) {
        camDist += 1.0f;
        glutPostRedisplay();
    }
}


// ============================================================================
// FUNCIÓN: mouseMovimiento()
//
// GLUT la llama CONTINUAMENTE mientras el mouse se mueve CON algún botón
// presionado (movimiento con clic).
// Para movimiento SIN clic se usaría glutPassiveMotionFunc() — no la usamos.
//
// Parámetros:
//   x, y → posición ACTUAL del cursor en la ventana
//
// LÓGICA PRINCIPAL:
//   1. Calculamos cuánto se movió el mouse desde la última vez (delta)
//   2. Usamos ese delta para rotar los ángulos de la cámara
//   3. Guardamos la posición actual como referencia para el próximo frame
// ============================================================================
void mouseMovimiento(int x, int y) {

    // Si el botón no está presionado, ignoramos el movimiento
    if (!mousePresionado) return;

    /*
     * CÁLCULO DEL DELTA (cuánto se movió el mouse):
     * dx = diferencia horizontal entre posición actual y última posición
     * dy = diferencia vertical entre posición actual y última posición
     *
     * Ejemplo: si lastMouseX=100 y ahora x=105, entonces dx=5
     * (el mouse se movió 5 píxeles hacia la derecha)
     */
    int dx = x - lastMouseX;
    int dy = y - lastMouseY;

    /*
     * SENSIBILIDAD: multiplicamos el delta por un factor pequeño (0.4f)
     * para que la rotación no sea demasiado brusca
     *
     * dx positivo (mouse va a la derecha) → camAngleX aumenta → cámara gira derecha
     * dy positivo (mouse va ABAJO en pantalla) → camAngleY aumenta → cámara sube
     * Nota: en GLUT, Y crece hacia ABAJO en la pantalla
     */
    camAngleX += dx * 0.4f;
    camAngleY += dy * 0.4f;

    /*
     * LÍMITES VERTICALES:
     * Evitamos que la cámara se vaya completamente arriba o abajo
     * Si camAngleY llega a 89°, la cámara miraría casi verticalmente
     * y el vector "arriba" se comportaría raro
     */
    if (camAngleY >  89.0f) camAngleY =  89.0f;
    if (camAngleY <   1.0f) camAngleY =   1.0f;

    /*
     * ACTUALIZAMOS la posición de referencia para el próximo movimiento
     * Sin esto, el delta se calcularía siempre desde el punto de inicio
     * y la rotación sería acumulativa y descontrolada
     */
    lastMouseX = x;
    lastMouseY = y;

    // Pedimos a GLUT que redibuje con los nuevos ángulos
    glutPostRedisplay();
}




// ============================================================================
// FUNCIÓN: cargarTexturasSkybox()
//
// Carga las 6 imágenes del espacio desde assets/
// Se llama UNA SOLA VEZ desde configurarOpenGL()
// ============================================================================
void cargarTexturasSkybox() {

    /*
     * Habilitamos el sistema de texturas 2D de OpenGL
     * Sin esto, glBindTexture no tiene efecto
     */
    glEnable(GL_TEXTURE_2D);

    // Cargamos cada cara del skybox
    // Ajusta los nombres si tus imágenes corresponden a caras diferentes
    skyboxTexturas[0] = cargarBMP("assets/1.bmp"); // derecha
    skyboxTexturas[1] = cargarBMP("assets/2.bmp"); // izquierda
    skyboxTexturas[2] = cargarBMP("assets/3.bmp"); // arriba
    skyboxTexturas[3] = cargarBMP("assets/4.bmp"); // abajo
    skyboxTexturas[4] = cargarBMP("assets/5.bmp"); // frente
    skyboxTexturas[5] = cargarBMP("assets/6.bmp"); // atrás

    // Desactivamos texturas al terminar
    // Las activaremos solo cuando dibujemos el skybox
    glDisable(GL_TEXTURE_2D);
}


// ============================================================================
// FUNCIÓN: dibujarSkybox()
//
// Dibuja un cubo gigante con las texturas del espacio en sus caras interiores.
//
// ORDEN DE OPERACIONES (muy importante):
// 1. Guardar estado actual
// 2. Desactivar depth write y lighting
// 3. Activar texturas
// 4. Dibujar las 6 caras del cubo con sus texturas
// 5. Restaurar todo
// ============================================================================
void dibujarSkybox() {

    float s = skyboxTamanio; // Abreviación para no repetir

    glPushMatrix();

    /*
     * Desactivamos la escritura al Z-buffer (pero no la lectura):
     * glDepthMask(GL_FALSE) → "no escribas profundidad mientras dibujas esto"
     *
     * ¿Por qué? Si el skybox escribe al Z-buffer, todos los objetos
     * de la escena quedarían "detrás" de él y no se verían.
     * Con esto desactivado, el skybox se dibuja pero no bloquea nada.
     */
    glDepthMask(GL_FALSE);

    /*
     * Desactivamos la iluminación:
     * El skybox siempre debe verse con su color/textura original
     * Si tuviera luz, se vería oscuro en algunas áreas → raro para un fondo
     */
    glDisable(GL_LIGHTING);


    /*
     * FORZAMOS culling OFF para el skybox:
     * Las caras del skybox miran hacia ADENTRO del cubo
     * Si el culling está activo, las eliminaría por "mirar al revés"
     * Guardamos el estado anterior para restaurarlo después
     */
    GLboolean cullActivo = glIsEnabled(GL_CULL_FACE);
    glDisable(GL_CULL_FACE);

    glEnable(GL_TEXTURE_2D);


    /*
     * Color blanco antes de dibujar con textura:
     * OpenGL MULTIPLICA el color del vértice por el color de la textura
     * Si el color es blanco (1,1,1), la textura se ve con sus colores reales
     * Si fuera rojo (1,0,0), la textura se vería rojiza
     */
    glColor3f(1.0f, 1.0f, 1.0f);

    /*
     * Coordenadas de textura (u, v) o (s, t):
     * Son coordenadas 2D que van de 0.0 a 1.0
     * Indican QUÉ PARTE de la textura mapear a cada vértice
     *
     * (0,0) = esquina inferior izquierda de la textura
     * (1,1) = esquina superior derecha de la textura
     *
     *  (0,1)-----(1,1)
     *    |         |
     *    |  imagen |
     *    |         |
     *  (0,0)-----(1,0)
     */

    // ========== CARA DERECHA (+X) ==========
    glBindTexture(GL_TEXTURE_2D, skyboxTexturas[0]);
    glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex3f( s, -s,  s);
        glTexCoord2f(1,0); glVertex3f( s, -s, -s);
        glTexCoord2f(1,1); glVertex3f( s,  s, -s);
        glTexCoord2f(0,1); glVertex3f( s,  s,  s);
    glEnd();

    // ========== CARA IZQUIERDA (-X) ==========
    glBindTexture(GL_TEXTURE_2D, skyboxTexturas[1]);
    glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex3f(-s, -s, -s);
        glTexCoord2f(1,0); glVertex3f(-s, -s,  s);
        glTexCoord2f(1,1); glVertex3f(-s,  s,  s);
        glTexCoord2f(0,1); glVertex3f(-s,  s, -s);
    glEnd();

    // ========== CARA ARRIBA (+Y) ==========
    glBindTexture(GL_TEXTURE_2D, skyboxTexturas[2]);
    glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex3f(-s,  s,  s);
        glTexCoord2f(1,0); glVertex3f( s,  s,  s);
        glTexCoord2f(1,1); glVertex3f( s,  s, -s);
        glTexCoord2f(0,1); glVertex3f(-s,  s, -s);
    glEnd();

    // ========== CARA ABAJO (-Y) - CORREGIDA ==========
    glBindTexture(GL_TEXTURE_2D, skyboxTexturas[3]);
    glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex3f(-s, -s,  s);  // CAMBIADO
        glTexCoord2f(1,0); glVertex3f( s, -s,  s);  // CAMBIADO
        glTexCoord2f(1,1); glVertex3f( s, -s, -s);  // CAMBIADO
        glTexCoord2f(0,1); glVertex3f(-s, -s, -s);  // CAMBIADO
    glEnd();

    // ========== CARA FRENTE (-Z) - CORREGIDA ==========
    glBindTexture(GL_TEXTURE_2D, skyboxTexturas[4]);
    glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex3f(-s, -s, -s);  // CAMBIADO
        glTexCoord2f(1,0); glVertex3f( s, -s, -s);  // CAMBIADO
        glTexCoord2f(1,1); glVertex3f( s,  s, -s);  // CAMBIADO
        glTexCoord2f(0,1); glVertex3f(-s,  s, -s);  // CAMBIADO
    glEnd();

    // ========== CARA ATRÁS (+Z) ==========
    glBindTexture(GL_TEXTURE_2D, skyboxTexturas[5]);
    glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex3f(-s, -s,  s);
        glTexCoord2f(1,0); glVertex3f( s, -s,  s);
        glTexCoord2f(1,1); glVertex3f( s,  s,  s);
        glTexCoord2f(0,1); glVertex3f(-s,  s,  s);
    glEnd();


	//Restauramos el estado del culling
	if (cullActivo) glEnable(GL_CULL_FACE);

    // ------------------------------------------------------------------
    // RESTAURAMOS EL ESTADO
    // ------------------------------------------------------------------
    glDisable(GL_TEXTURE_2D);  // Apagamos texturas
    glDepthMask(GL_TRUE);       // Reactivamos escritura al Z-buffer
    glEnable(GL_LIGHTING);      // Reactivamos iluminación

    glPopMatrix();
}




// ============================================================================
// FUNCIÓN: main()
// Punto de entrada del programa
// ============================================================================
int main(int argc, char** argv) {
    
    // Inicializar GLUT con los argumentos de la línea de comandos
    glutInit(&argc, argv);

    /*
     * glutInitDisplayMode(mode):
     * Configura el modo de visualización inicial
     * 
     * Flags usados:
     * - GLUT_DOUBLE: Doble buffer (dibuja fuera de pantalla, luego copia)
     *                → Evita parpadeo (flickering)
     * - GLUT_RGB: Modo de color RGB (rojo, verde, azul)
     * - GLUT_DEPTH: Buffer de profundidad (Z-buffer)
     *               → Para saber qué objeto está adelante/atrás
     * - GLUT_STENCIL: Buffer de stencil
     *                 → Para efectos avanzados (reflejos, sombras)
     */
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);

    // Tamaño inicial de la ventana
    glutInitWindowSize(800, 600);

    // Crea la ventana con nombre
    glutCreateWindow("ProyectoFinal: caja enigma");

    /*
     * glClearColor(r, g, b, a):
     * Define el color de fondo cuando se "limpia" la pantalla
     * Valores: 0.0 (oscuro) a 1.0 (claro)
     * - (0.1f, 0.1f, 0.1f): Gris muy oscuro (casi negro)
     * - (1.0f): Alpha = completamente opaco
     */
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    /*
     * Registrar CALLBACKS: funciones que GLUT llamará automáticamente en eventos
     */
    glutDisplayFunc(display);       // Llamada cada vez que se redibuja
    glutReshapeFunc(myReshape);     // Llamada cuando la ventana cambia de tamaño


     /*
     * Registramos los CALLBACKS de controles:
     *
     * glutKeyboardFunc   → teclas normales (letras, números, escape)
     * glutSpecialFunc    → teclas especiales (flechas, F1-F12)
     * glutMouseFunc      → clic/soltar botones del mouse
     * glutMotionFunc     → movimiento del mouse CON botón presionado
     */
    glutKeyboardFunc(teclado);
    glutSpecialFunc(tecladoEspecial);
    glutMouseFunc(mouseBoton);
    glutMotionFunc(mouseMovimiento);


				    
    configurarOpenGL(); //-> preparamos luces y materiales antes del loop

    /*
     * glutMainLoop():
     * Inicia el CICLO INFINITO DE EVENTOS
     * El programa se queda aquí hasta que cierres la ventana
     * Continuamente:
     * 1. Dibuja
     * 2. Procesa eventos del mouse/teclado
     * 3. Llama a funciones registradas (callbacks)
     * 4. Repite
     */
    glutMainLoop();

    return 0;
}

// ============================================================================
// GLOSARIO: SISTEMA DE COORDENADAS EN OPENGL
// ============================================================================
/*
 * Eje X: Horizontal (paralelo a la pantalla)
 *        - Negativo (-X): izquierda
 *        - Positivo (+X): derecha
 *
 * Eje Y: Vertical (paralelo a la pantalla)
 *        - Negativo (-Y): abajo
 *        - Positivo (+Y): arriba
 *
 * Eje Z: Profundidad (perpendicular a la pantalla)
 *        - Negativo (-Z): hacia afuera (hacia ti)
 *        - Positivo (+Z): hacia adentro (lejos de ti)
 *
 * ORIGEN (0, 0, 0): Centro del mundo
 */

// ============================================================================
// NOTAS SOBRE EL CÓDIGO
// ============================================================================
/*
 * Este código es el ESQUELETO del proyecto:
 * - PASO 1: Dibuja un piso (HECHO)
 * - PASO 2: Agregar iluminación (PRÓXIMO)
 * - PASO 3: Agregar cubo/caja (PRÓXIMO)
 * - PASO 4: Agregar articulaciones (PRÓXIMO)
 * - PASO 5: Agregar controles (PRÓXIMO)
 * 
 * IMPORTANTE: Cada paso es independiente. Verifica que cada uno funcione
 * antes de pasar al siguiente.
 *
 *
 * ### Resumen de controles:
```
Mouse izquierdo + arrastrar  →  rotar cámara
Scroll arriba/abajo          →  zoom in/out
W / S                        →  zoom in/out (alternativo)
Flechas ←→↑↓                →  rotar cámara (alternativo)
C                            →  culling on/off
D                            →  depth test on/off
Z                            →  wireframe on/off
X                            →  smooth/flat shading
Escape                       →  cerrar
F1-F4                        →  mismos que C,D,Z,X
 */
