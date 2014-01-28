//#include <iostream> // keypresses
#include <fstream> // read from txt file
#include <string>
const float M_PI = 3.14159265358979323846f;

namespace octet {


	class LSystem
	{
		std::string axiom;
		std::string finalString;         //This is the string to output the iteration       
		std::string rules[2][5];		// assuming that there is never more that 5 rules
			
		int pos;
		vec4 twigColor;
		vec4 leafColor;
		float angle;
		int iterationNum;

		vec3 point;
		dynarray<vec3> point_stack;//Position stack for branching, this is used for going to previous node
		dynarray<vec3> vertices;//store all lines verticesin stack
		dynarray<vec3> leafVertices;//store all vertices of leafs in stack

		mat4t modelToWorld;
		int branchLength;
		int branchWidth;
		float x_delta;
		float y_delta;

		int rulesNum;
		int leafSize;
		bool drawLeafs;


		
	public:	
		//constructor
		LSystem(float posX, float posY, std::string rule[2][5], int iterations, float angle_, std::string axiomM, int rulesCount, int branchLength_, int branchWidth_, bool drawLeafs_)
		{
			modelToWorld.loadIdentity();
			
			iterationNum = iterations;
			axiom = axiomM;
			finalString = axiomM;
			point[0] = 0;     //X 
			point[1] = 0;     //Y 
			point[2] = 0;	  //Angle
    
			twigColor = vec4(0.44f, 0.32f, 0.27f, 1);
			leafColor = vec4(0, 0.58f, 0.3f, 1);
			leafSize = (branchLength_/3)+branchWidth_;//make leaf size proportional to branch length and branch width
			pos = 0;
			angle = (M_PI/180)*angle_;
			rulesNum =rulesCount;
			branchLength = branchLength_;
			branchWidth = branchWidth_;

			drawLeafs = drawLeafs_;
			
			memcpy(rules, rule, sizeof rules);

			modelToWorld.translate(posX, posY+20, 0);//translate the tree to the middle bottom of the screen
		}
		
		/*****************************************************************
		Method that iterates through the final string (iterationNum times) 
		in order to rewrite specific symbols according to  production rules
		******************************************************************/
		void composeString()
		{
			for(int i = 0; i < iterationNum; i++)
			{
				std::string next_str = "";
				for(int j = 0; j < finalString.length(); j++)
				{
					char c = finalString[j];
					int k = 0;
					for(; k < rulesNum; k++)
					{
						if(c == (rules[0][k])[0])//compares c with first element of each rule (one that is before =)
						{
							next_str += rules[1][k];//appends a second part of rules (after =) to the next_str
							break;
						}
					}
					if(k == 2)
						next_str += c;
					
				}
				finalString = next_str;
			}

			composeVertices();
		}

		/*****************************************************************
		Method that iterates through the final string and translates the 
		caracters into a geometric vertices:
		F -> create a line with positions
		- -> change the angle to the negative direction
		+ -> change the angle to the positive direction
		[ -> push the current position to a stack
		] -> pop the position and set previous position as current 
		push and pop are used to create branches of the plant
		******************************************************************/	
		void composeVertices()
		{
			while(pos<finalString.length())
			{
				
				char c = finalString[pos];
				switch(c)
				{ 
				case 'F':
					forward();
					break;
				case 'L':
					if(drawLeafs)
					{
						addLeaf();
					}
					break;
				case '-':
					angleMinus();
					break;
				case '+':
					anglePlus();
					break;				
				case '[':
					push();
					break;
				case ']':
					pop();
					break;
				}
				pos++;
			}	
		}	

		/**********************************************************************
		Method that creates the new line by storing two positions
		x1, y1 and x2, y2 in the stack
		the second position is found by the branch length times angle sin or cos
		***********************************************************************/	
		void forward()
		{
			x_delta = branchLength * -sin(point[2]);
			y_delta = branchLength * cos(point[2]);
			vertices.push_back(point);//x1, y1
			vertices.push_back(vec3(point[0]+x_delta, point[1]+y_delta, 0));//x2, y2
			point[0]= point[0]+x_delta;
			point[1]= point[1]+y_delta;
		}

		/**********************************************************************
		Method that increases angle
		***********************************************************************/
		void anglePlus()
		{
			point[2] += angle;
		}

		/**********************************************************************
		Method that decreases angle
		***********************************************************************/
		void angleMinus()
		{
			point[2] -= angle;
		}

		/**********************************************************************
		Method that push current point to stack
		***********************************************************************/
		void push()
		{
			point_stack.push_back(point);
		}

		/**********************************************************************
		Method that pop the point from the stack and sets the current point 
		to a preveous point value
		***********************************************************************/
		void pop()
		{
			point = point_stack[point_stack.size() - 1];
			point_stack.pop_back();			
		}

		/**********************************************************************
		Method that adds a leaf vertices to the stack (6 vertices to create a square)
		***********************************************************************/
		void addLeaf()
		{			 
			leafVertices.push_back(point);//x1, y1
			leafVertices.push_back(vec3(point[0]-leafSize, point[1], 0));//x2, y2
			leafVertices.push_back(vec3(point[0]-leafSize, point[1]+leafSize, 0));//x3, y3
			leafVertices.push_back(point);//x1, y1
			leafVertices.push_back(vec3(point[0]-leafSize, point[1]+leafSize, 0));//x3, y3
			leafVertices.push_back(vec3(point[0], point[1]+leafSize, 0));//x4, y4
		}

		/**********************************************************************
		Method that draws all the lines as one mesh and all the leaf vertices 
		as the other mesh
		***********************************************************************/
		void draw(color_shader &shader, mat4t &cameraToWorld)
		{	
			modelToWorld.rotateY(1.5f);
						
			glLineWidth(GLfloat(branchWidth));//control the width of the lines
			mat4t modelToProjection = mat4t::build_projection_matrix(modelToWorld, cameraToWorld);

			//draw lines
			shader.render(modelToProjection, twigColor);
			glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), &vertices[0][0] );
			glEnableVertexAttribArray(attribute_pos);    
			glDrawArrays(GL_LINES, 0, vertices.size());
			//make lines smooth
			glEnable( GL_LINE_SMOOTH );
			glHint( GL_LINE_SMOOTH_HINT, GL_NICEST );

			if(drawLeafs)
			{
				//draw leafs			
				shader.render(modelToProjection, leafColor);
				glVertexAttribPointer(attribute_pos, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), &leafVertices[0][0] );
				glEnableVertexAttribArray(attribute_pos);
				glDrawArrays(GL_TRIANGLES, 0, leafVertices.size());
			}
		}

		/**********************************************************************
		Method that is used to update and reset all the values
		***********************************************************************/
		void update(std::string rule[2][5], int iterations, float angle_, std::string axiomM, int branchLength_, int branchWidth_, bool drawLeafs_)
		{
			iterationNum = iterations;
			axiom = axiomM;
			finalString = axiomM;
			point[0] = 0; 
			point[1] = 0;  
			point[2] = 0;	
    
			pos = 0;
			angle = (M_PI/180)*angle_;
			branchLength = branchLength_;
			branchWidth = branchWidth_;
			leafSize = (branchLength_/3)+branchWidth_;

			drawLeafs = drawLeafs_;

			memcpy(rules, rule, sizeof rules);

			//reset all stacks
			point_stack.reset();
			vertices.reset();
			leafVertices.reset();
		}
		
	};


  class lsystem_app : public octet::app {

    // Matrix to transform points in our camera space to the world.
    // This lets us move our camera
    mat4t cameraToWorld;
		  
	// shader to draw a solid color
    color_shader color_shader_;
	std::string lines[10];
	std::string axiom;
	int iterations;
	float angle;
	std::string rules[2][5];
	int rulesCounter;
	LSystem *ls;

	unsigned int timeCount;
	unsigned int prevTimeCount;

	int branchLength;
	int branchWidth;

	bool drawLeaf;
	
   public:

		// this is called when we construct the class
		lsystem_app(int argc, char **argv) : app(argc, argv) 
		{	
			prevTimeCount = 0;
			branchLength = 5;
			branchWidth = 1;
			drawLeaf = true;
			readFile("c:\\rule1.txt");	
			ls = new LSystem(512.0f, 0, rules, iterations, angle, axiom, rulesCounter, branchLength, branchWidth, drawLeaf);
			ls->composeString();
		}
		
		/**********************************************************************
		Method that reads the .txt file and stores all the lines in array
		***********************************************************************/
		void readFile(std::string filename)
		{		
			fstream inFile;
			inFile.open(filename, std::ios::in);
			int count = 0;
			if(inFile)
			{
				while(inFile){
					std::getline(inFile, lines[count]);
					count++;
				}
				inFile.close();
				printf("file loaded\n");
				readString();
			}
			else
			{
				printf("Unable to open file");
			}
		}

		/**********************************************************************
		Method that reads all the lines in the string array and reads in the 
		axiom, ireation number, angle and rules
		***********************************************************************/
		void readString()
		{
			rulesCounter =0;
			for(int i = 0; i<10; i+=2)//read every second line
			{
			
				if(lines[i] == "Axiom")
				{
					axiom = lines[i+1];
				}
				else if(lines[i] == "Iterations")
				{
					iterations = std::stoi(lines[i+1]);
				}
				else if(lines[i] == "Angle")
				{
					angle = std::stof(lines[i+1]);
				}
				else if(lines[i] == "Rule")
				{
					char* token;
					token = strtok((char*)lines[i+1].c_str(), "=");//splitting the line into to parts 
					rules[0][rulesCounter] = token;//everything before =
					token = strtok(NULL, "=");
					rules[1][rulesCounter] = token;//everything after =
					rulesCounter++;
				}
			}
		}


		/**********************************************************************
		Method that reads the keyboard inputs:
		-iterations
		-branch length
		-branch width
		-angle
		-draw leaf or not
		-configuration files
		***********************************************************************/
		void keyboard()
		{
			//timer count for keyboard 
			if (prevTimeCount - timeCount < -100)
			{
				//control iterations 
				if (is_key_down(key_up))
				{
					iterations++;
					ls->update(rules, iterations, angle, axiom, branchLength, branchWidth, drawLeaf);
					ls->composeString();
					prevTimeCount = timeCount;
				} 
				else if (is_key_down(key_down )) 
				{
					iterations--;
					ls->update(rules, iterations, angle, axiom, branchLength, branchWidth, drawLeaf);
					ls->composeString();
					prevTimeCount = timeCount;
				}
				//control angle
				if (is_key_down(key_right))
				{
					angle++;
					ls->update(rules, iterations, angle, axiom, branchLength, branchWidth, drawLeaf);
					ls->composeString();
					prevTimeCount = timeCount;
				} 				
				else if (is_key_down(key_left)) 
				{
					angle--;
					ls->update(rules, iterations, angle, axiom, branchLength, branchWidth, drawLeaf);
					ls->composeString();
					prevTimeCount = timeCount;
				}
				//control branch length
				else if (is_key_down(key_lmb)&& branchLength>0) //update only if branchLength is more than 0
				{	
					branchLength--;
					ls->update(rules, iterations, angle, axiom, branchLength, branchWidth, drawLeaf);
					ls->composeString();
					prevTimeCount = timeCount;
				}
				if (is_key_down(key_mmb))
				{
					branchLength++;
					ls->update(rules, iterations, angle, axiom, branchLength, branchWidth, drawLeaf);
					ls->composeString();
					prevTimeCount = timeCount;
				} 
				//control branch width
				else if (is_key_down('9') && branchWidth>0) //update only if branchWidth is more than 0
				{
					branchWidth--;					
					ls->update(rules, iterations, angle, axiom, branchLength, branchWidth, drawLeaf);
					ls->composeString();
					prevTimeCount = timeCount;
				}
				if (is_key_down('0'))
				{
					branchWidth++;
					ls->update(rules, iterations, angle, axiom, branchLength, branchWidth, drawLeaf);
					ls->composeString();
					prevTimeCount = timeCount;
				} 
				//toggle between drawing leaf and not drawing leaf
				else if (is_key_down(key_shift)) //update only if branchWidth is more than 0
				{
					drawLeaf = !drawLeaf;				
					ls->update(rules, iterations, angle, axiom, branchLength, branchWidth, drawLeaf);
					ls->composeString();
					prevTimeCount = timeCount;
				}
				//control configuration files
				else if (is_key_down('1')) 
				{
					readFile("c:\\rule1.txt");
					ls->update(rules, iterations, angle, axiom, branchLength, branchWidth, drawLeaf);
					ls->composeString();
					prevTimeCount = timeCount;
				}
				else if (is_key_down('2')) 
				{
					readFile("c:\\rule2.txt");
					ls->update(rules, iterations, angle, axiom, branchLength, branchWidth, drawLeaf);					
					ls->composeString();
					prevTimeCount = timeCount;
				}
				else if (is_key_down('3')) 
				{
					readFile("c:\\rule3.txt");
					ls->update(rules, iterations, angle, axiom, branchLength, branchWidth, drawLeaf);
					prevTimeCount = timeCount;
					ls->composeString();
				}
				else if (is_key_down('4')) 
				{
					readFile("c:\\rule4.txt");
					ls->update(rules, iterations, angle, axiom, branchLength, branchWidth, drawLeaf);					
					ls->composeString();
					prevTimeCount = timeCount;
				}
				else if (is_key_down('5')) 
				{
					readFile("c:\\rule5.txt");
					ls->update(rules, iterations, angle, axiom, branchLength, branchWidth, drawLeaf);					
					ls->composeString();
					prevTimeCount = timeCount;
				}
				else if (is_key_down('6')) 
				{
					readFile("c:\\rule6.txt");
					ls->update(rules, iterations, angle, axiom, branchLength, branchWidth, drawLeaf);					
					ls->composeString();
					prevTimeCount = timeCount;
				}
			}
		}

		/**********************************************************************
		Method that is called once OpenGL is initialized
		***********************************************************************/	
		void app_init()
		{
			color_shader_.init();
			cameraToWorld.loadIdentity();
			cameraToWorld.translate(512, 512, 512);		
		}

		/**********************************************************************
		Method that is called every frame
		***********************************************************************/
		void draw_world(int x, int y, int w, int h) 
		{
			timeCount = GetTickCount();//update current time
			keyboard();//read keyboard inputs
			// set a viewport - includes whole window area
			glViewport(x, y, w, h);

			// clear the background to white
			glClearColor(1, 1, 1, 1);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// allow Z buffer depth testing (closer objects are always drawn in front of far ones)
			glEnable(GL_DEPTH_TEST);

			ls->draw(color_shader_, cameraToWorld);
		}

	};
	
  }