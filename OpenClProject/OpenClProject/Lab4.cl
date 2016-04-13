__kernel void OpenCLHeatTransfer(__global float* previousMatrix, __global float* currentMatrix, int rowCount, int columnCount, float td, float h)
{
	int id = get_global_id(0);

	int column = id % columnCount;
	int line = id / columnCount;
	
	if (column == 0 || column == columnCount - 1 || line == 0 || line == rowCount - 1)
	{
		return;
	}

	int correctID = line * columnCount + column;

	currentMatrix[correctID] = (1 - 4 * td / (h*h)) * previousMatrix[correctID] + (td / (h*h)) * (previousMatrix[correctID - columnCount] + previousMatrix[correctID + columnCount] + previousMatrix[correctID - 1] + previousMatrix[correctID + 1]);
}