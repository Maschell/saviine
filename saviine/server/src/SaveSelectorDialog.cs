using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace saviine_server
{
    public partial class SaveSelectorDialog : Form
    {
        private string newPath = "";

        public string NewPath
        {
            get { return newPath; }
        }
        private static string savePath = Program.root + "/" + "inject";
        public SaveSelectorDialog(string path,string title_id)
        {
            InitializeComponent();
            string[] stringSeparators = new string[] { "vol/save/", "vol\\save\\" };
            string[] result;

            result = path.Split(stringSeparators, StringSplitOptions.None);
            if (result.Length < 2) this.Close();
            string resultPath = result[result.Length-2];

            Console.WriteLine(title_id);
            savePath += "/" + title_id;
            if (Directory.Exists(savePath))
            {
                // Recurse into subdirectories of this directory.
                string[] subdirectoryEntries = Directory.GetDirectories(savePath);
                foreach (string subdirectory in subdirectoryEntries)
                {
                    this.listBox_saves.Items.Add(Path.GetFileName(subdirectory));
                }
            }
            else
            {
                Console.WriteLine("dir not found! " + savePath);
                this.Close();
            }          
           
        }

        private void Form1_Load(object sender, EventArgs e)
        {

        }

        private void btn_ok_Click(object sender, EventArgs e)
        {
            newPath = savePath + "/" + this.listBox_saves.SelectedItem.ToString();
        }
        private void btn_cancel_Click(object sender, EventArgs e)
        {
            
        }
        
      
    }
}
