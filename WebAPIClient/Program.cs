
using System.Threading;

internal class Program
{
    private static async Task Main(string[] args)
    {
        using HttpClient client = new();

        while (true)
        {
            await ProcessONWebRequestAsync(client);
            Thread.Sleep(15000);
            await ProcessOFFWebRequestAsync(client);
            Thread.Sleep(15000);


        }
        static async Task ProcessONWebRequestAsync(HttpClient client)
        {
            var resp = await client.GetStringAsync(
              "http://192.168.0.54/ON");

            Console.Write(resp);

        }

        static async Task ProcessOFFWebRequestAsync(HttpClient client)
        {
            var resp = await client.GetStringAsync(
              "http://192.168.0.54/OFF");

            Console.Write(resp);
        }
    }
}