package ludii_cpp_ai;

import app.StartDesktopApp;
import manager.ai.AIRegistry;

/**
 * Class with main method to launch Ludii's GUI with our C++-based AI registered.
 * 
 * @author Dennis Soemers
 */
public class Main 
{
	
	/**
	 * Our main method.
	 * 
	 * @param args
	 */
	public static void main(final String[] args)
	{
		if (!AIRegistry.registerAI("Kriegspiel C++ Agent Example", () -> {return new LudiiKriegspielCppAI();}, (game) -> {return new LudiiKriegspielCppAI().supportsGame(game);}))
			System.err.println("WARNING! Failed to register AI because one with that name already existed!");
		
		StartDesktopApp.main(new String[0]);
	}

}
