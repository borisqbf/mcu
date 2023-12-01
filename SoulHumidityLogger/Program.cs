using System;
using System.ComponentModel;
using System.IO;
using System.Net.Http;
using System.Threading.Tasks;

class Program
{
    private const string humidityApiUrl = "http://irrigation-controller.local/humidity";
    private const string tankLevelApiUrl = "http://rain-tank-level.local/api/level-abs";

    private const string filePath = "c:/temp/humidity-log.csv"; // Change this to your desired file path

    static async Task Main()
    {
        try
        {
            // Make the HTTP GET request to the API
            string humidityApiResponse = await GetApiValueAsync(humidityApiUrl);
            string tankLevelApiResponse = await GetApiValueAsync(tankLevelApiUrl);
            humidityApiResponse = humidityApiResponse.Remove(0, 10);
            int humidity = Int32.Parse(humidityApiResponse.Replace("\n", ""));

            // Combine the API response with a timestamp
            string logEntry = $"{DateTime.Now:yyyy-MM-dd HH:mm:ss},{humidity},{tankLevelApiResponse}";

            // Append the log entry to the text file
            File.AppendAllText(filePath, logEntry + Environment.NewLine);

            Console.WriteLine(logEntry);

        }
        catch (Exception ex)
        {
            Console.WriteLine($"An error occurred: {ex.Message}");
        }
    }


    static async Task<string> GetApiValueAsync(string apiUrl)
    {
        using (HttpClient client = new HttpClient())
        {
            // Make the GET request
            HttpResponseMessage response = await client.GetAsync(apiUrl);

            // Check if the request was successful
            response.EnsureSuccessStatusCode();

            // Read the content of the response
            return await response.Content.ReadAsStringAsync();
        }
    }
}