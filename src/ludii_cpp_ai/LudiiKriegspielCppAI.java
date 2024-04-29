package ludii_cpp_ai;

import java.io.File;

import game.Game;
import other.AI;
import other.context.Context;
import other.move.Move;


/**
 * Example Java wrapper for a Ludii Kriegspiel AI implemented in C++
 *
 * @author Nikola Novarlic
 */
public class LudiiKriegspielCppAI extends AI
{
	
	//-------------------------------------------------------------------------
	
	/** 
	 * Name of our native library with C++-based AI code.
	 * NOTE: should not include any file extension or path, just the name (without lib prefix on Linux/MacOS)!
	 */
	protected static final String NATIVE_LIB_FILE = "LudiiKriegspielCppAI";
	
	/** Our player index */
	protected int player = -1;
	
	/** Opponent player index **/
	protected int opponent = -1;
	
	/** Number of players **/
	private final int players = 2;
	
	static
	{
		try
		{
			// Make sure our native code is loaded
			System.loadLibrary(NATIVE_LIB_FILE);
		}
		catch (final java.lang.UnsatisfiedLinkError e)
		{
			System.err.println("Failed to load library: " + NATIVE_LIB_FILE);
			System.err.println("java.library.path currently points to: " + new File(System.getProperty("java.library.path")).getAbsolutePath());
			System.err.println("You can change this path by using the '-Djava.library.path=...' VM argument.");
			throw e;
		}
		
		// Perform C++ side init
		nativeStaticInit();
	}
	
	//-------------------------------------------------------------------------
	
	/**
	 * Constructor
	 */
	public LudiiKriegspielCppAI()
	{
		this.friendlyName = "Kriegspiel C++ Agent Example";
	}
	
	//-------------------------------------------------------------------------

	@Override
	public Move selectAction
	(
		final Game game, 
		final Context context, 
		final double maxSeconds,
		final int maxIterations,
		final int maxDepth
	)
	{
		String lastTryMessages = String.join(";", context.getNotes(player));
		String refereeMessages = String.join(";", context.getNotes(opponent));
		
		// Call the native function to return an action
		return nativeSelectAction(game, context, maxSeconds, maxIterations, maxDepth, lastTryMessages, refereeMessages);
	}
	
	@Override
	public void initAI(final Game game, final int playerID)
	{
		this.player = playerID;
		this.opponent = this.players - playerID + 1;
		
		// Perform any C++-side init
		nativeInitAI(game, playerID);
	}
	
	@Override
	public void closeAI()
	{
		// Call the native closeAI() method
		nativeCloseAI();
	}
	
	@Override
	public boolean supportsGame(final Game game)
	{
		return nativeSupportsGame(game);
	}
	
	//-------------------------------------------------------------------------
	
	/**
	 * Native (C++) version of selectAction()
	 * 
	 * @param game
	 * @param context
	 * @param maxSeconds
	 * @param maxIterations
	 * @param maxDepth
	 * @param lastTryMessages,
	 * @param refereeMessages
	 * @return
	 */
	private native Move nativeSelectAction
	(
		final Game game, 
		final Context context, 
		final double maxSeconds,
		final int maxIterations,
		final int maxDepth,
		final String lastTryMessages,
		final String refereeMessages
	);
	
	/**
	 * Native (C++) version of initAI()
	 * 
	 * @param game
	 * @param playerID
	 */
	private native void nativeInitAI(final Game game, final int playerID);
	
	/**
	 * Native (C++) version of closeAI()
	 */
	private native void nativeCloseAI();
	
	/**
	 * Native (C++) version of supportsGame()
	 * @param game
	 * @return
	 */
	private native boolean nativeSupportsGame(final Game game);
	
	/**
	 * Let our native code do some initial static (game-independent) initialisation.
	 */
	private native static void nativeStaticInit();
	
	//-------------------------------------------------------------------------

}
