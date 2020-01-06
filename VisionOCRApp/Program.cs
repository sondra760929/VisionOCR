using Google.Cloud.Vision.V1;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace VisionOCRApp
{
    class Program
    {
        static void Main(string[] args)
        {
            int argCount = args.Length;
            if(argCount > 2)
            {
                string input_file = args[0];
                string output_file = args[1];
                int current_type;
                Int32.TryParse(args[2], out current_type);
                File.Delete(output_file);

                if (current_type == 0)
                {
                    var image = Google.Cloud.Vision.V1.Image.FromFile(input_file);
                    var client = ImageAnnotatorClient.Create();
                    var response = client.DetectText(image);
                    var ocr_text = new StringBuilder();
                    foreach (var annotation in response)
                    {
                        if (annotation.Description != null)
                        {
                            ocr_text.AppendLine(annotation.Description);
                            break;
                        }
                    }

                    File.WriteAllText(output_file, ocr_text.ToString(), Encoding.Default);
                }
                else if(current_type == 1)
                {
                    var image = Google.Cloud.Vision.V1.Image.FromFile(input_file);
                    var client = ImageAnnotatorClient.Create();
                    var response = client.DetectDocumentText(image);
                    var ocr_text = new StringBuilder();
                    foreach(var page in response.Pages)
                    {
                        foreach(var block in page.Blocks)
                        {
                            var ocr_paragraph = new StringBuilder();
                            foreach (var paragraph in block.Paragraphs)
                            {
                                foreach(var word in paragraph.Words)
                                {
                                    //Console.WriteLine($"    Word: {string.Join("", word.Symbols.Select(s => s.Text))}");
                                    ocr_paragraph.Append(string.Join("", word.Symbols.Select(s => s.Text)));
                                    ocr_paragraph.Append(" ");
                                }
                            }
                            ocr_text.AppendLine(ocr_paragraph.ToString());
                        }
                        ocr_text.AppendLine("");
                    }
                    File.WriteAllText(output_file, ocr_text.ToString(), Encoding.Default);
                }
            }
        }
    }
}
