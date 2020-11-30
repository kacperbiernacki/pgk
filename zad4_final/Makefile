default: instancing_quads

%: %.cpp
	g++ -I. -std=c++17 $< -o $@ -lGLEW  -lGL -lglfw -lepoxy

clean:
	rm a.out *.o *~ instancing_quads
	
run:
	./instancing_quads
