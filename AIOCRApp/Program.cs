using Newtonsoft.Json.Linq;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net;
using System.Text;

namespace AIOCRApp
{
    class FieldData
    {
        public List<Point> points = new List<Point>();
        public string text;
        public bool line_break;
    }
    class Program
    {
        static string accessKey = "RFV6Q1RPSnRlbElzelljdHRoeEVUWlZaZGRFWnZuT0U=";//"bnBrbU5NRWJBakZYTnRJTWJoY1NpV1ZwenFEYllEaHM=";
        static string uriBase = "https://bd61323638684688a1e648de03a65c1f.apigw.ntruss.com/custom/v1/12350/9b3ed5ef657408042e07bb6deb64648dd11faf84671df3bdb5bb9d19dc463da8/general";//"https://43d0993faa7143c0add4cbdfa7fac53d.apigw.ntruss.com/custom/v1/12325/800a9d7344b09c6a7ea43f71842cf3473eb134405af99c384a5a5c8c204b39d2/general";

        static void Main(string[] args)
        {
            int argCount = args.Length;
            if (argCount > 2)
            {
                string input_file = args[0];
                string output_file = args[1];
                int current_type;
                Int32.TryParse(args[2], out current_type);
                File.Delete(output_file);

                FileInfo fi = new FileInfo(input_file);
                if (fi.Exists)
                {
                    //if (current_type == 0)
                    //{
                        var ocr_text = new StringBuilder();
                        string text_line = "";
                        string json = DoOCR(fi.FullName, fi.Name, fi.Extension.ToLower());
                        List<FieldData> test = GetFields(json);
                        for (int i = 0; i < test.Count; i++)
                        {
                            text_line += test[i].text;
                            text_line += " ";
                            if (test[i].line_break)
                            {
                                ocr_text.AppendLine(text_line);
                                text_line = "";
                            }
                        }
                        if(text_line != "")
                        {
                            ocr_text.AppendLine(text_line);
                        }

                        File.WriteAllText(output_file, ocr_text.ToString(), Encoding.Default);
                    //}
                    //else if (current_type == 1)
                    //{
                    //    var image = Google.Cloud.Vision.V1.Image.FromFile(input_file);
                    //    var client = ImageAnnotatorClient.Create();
                    //    var response = client.DetectDocumentText(image);
                    //    var ocr_text = new StringBuilder();
                    //    foreach (var page in response.Pages)
                    //    {
                    //        foreach (var block in page.Blocks)
                    //        {
                    //            var ocr_paragraph = new StringBuilder();
                    //            foreach (var paragraph in block.Paragraphs)
                    //            {
                    //                foreach (var word in paragraph.Words)
                    //                {
                    //                    //Console.WriteLine($"    Word: {string.Join("", word.Symbols.Select(s => s.Text))}");
                    //                    ocr_paragraph.Append(string.Join("", word.Symbols.Select(s => s.Text)));
                    //                    ocr_paragraph.Append(" ");
                    //                }
                    //            }
                    //            ocr_text.AppendLine(ocr_paragraph.ToString());
                    //        }
                    //        ocr_text.AppendLine("");
                    //    }
                    //    File.WriteAllText(output_file, ocr_text.ToString(), Encoding.Default);
                    //}
                }
            }
        }

        static List<FieldData> GetFields(string json)
        {
            List<FieldData> return_value = new List<FieldData>();

            JObject root = JObject.Parse(json);
            if (root.Count > 0)
            {
                JArray images = (JArray)root["images"];
                if (images != null)
                {
                    for (int i = 0; i < images.Count; i++)
                    {
                        JObject image = (JObject)images[i];
                        if (image != null && image.Count > 0)
                        {
                            JValue inferResult = (JValue)image["inferResult"];
                            if (inferResult != null && inferResult.ToString() == "SUCCESS")
                            {
                                JArray fields = (JArray)image["fields"];
                                for (int j = 0; j < fields.Count; j++)
                                {
                                    JObject field = (JObject)fields[j];
                                    if (field.Count > 0)
                                    {
                                        FieldData fd = new FieldData();
                                        JObject boundingPoly = (JObject)field["boundingPoly"];
                                        if (boundingPoly.Count > 0)
                                        {
                                            JArray vertices = (JArray)boundingPoly["vertices"];
                                            for (int k = 0; k < vertices.Count; k++)
                                            {
                                                Point p = new Point(int.Parse(vertices[k]["x"].ToString()), int.Parse(vertices[k]["y"].ToString()));
                                                fd.points.Add(p);
                                            }
                                        }
                                        JValue inferText = (JValue)field["inferText"];
                                        fd.text = inferText.ToString();

                                        JValue lineBreak = (JValue)field["lineBreak"];
                                        fd.line_break = (bool)lineBreak.Value;

                                        return_value.Add(fd);
                                    }
                                }
                            }
                            else
                            {
                                //	ocr error
                            }
                        }
                    }
                }
            }

            return return_value;
        }

        static long UnixTimeNow()
        {
            var timeSpan = (DateTime.UtcNow - new DateTime(1970, 1, 1, 0, 0, 0));
            return (long)timeSpan.TotalSeconds;
        }

        static string DoOCR(string imagePath, string file_name, string file_ext)
        {
            var imageBinary = GetImageBinary(imagePath);

            string guid = Guid.NewGuid().ToString();
            guid = guid.Replace("-", string.Empty);
            file_ext = file_ext.Replace(".", string.Empty);

            HttpWebRequest webRequest = (HttpWebRequest)WebRequest.Create(uriBase);
            webRequest.Method = "POST";
            webRequest.Headers.Add("X-OCR-SECRET", accessKey);
            webRequest.Timeout = 30 * 1000; // 30초 
                                            // ContentType은 지정된 것이 있으면 그것을 사용해준다. 
            webRequest.ContentType = "application/json; charset=utf-8";
            // json을 string type으로 입력해준다. 
            string postData = "{\"version\": \"V2\"," +
                "\"requestId\": \"" + guid + "\"," +
                "\"timestamp\": " + UnixTimeNow().ToString() + "," +
                "\"lang\": \"ko\"," +
                "\"images\": [{ \"format\": \"" + file_ext + "\", \"data\": \"" + Convert.ToBase64String(imageBinary) + "\", \"name\": \"" + file_name + "\"}]}";
            // 보낼 데이터를 byteArray로 바꿔준다. 
            byte[] byteArray = Encoding.UTF8.GetBytes(postData);
            // 요청 Data를 쓰는 데 사용할 Stream 개체를 가져온다. 
            Stream dataStream = webRequest.GetRequestStream();
            // Data를 전송한다.
            dataStream.Write(byteArray, 0, byteArray.Length);
            // dataStream 개체 닫기 
            dataStream.Close();

            string responseText = string.Empty;
            using (WebResponse resp = webRequest.GetResponse())
            {
                Stream respStream = resp.GetResponseStream();
                using (StreamReader sr = new StreamReader(respStream))
                {
                    responseText = sr.ReadToEnd();
                }
            }

            return responseText;
        }
        static byte[] GetImageBinary(string path)
        {
            return File.ReadAllBytes(path);
        }
    }
}
