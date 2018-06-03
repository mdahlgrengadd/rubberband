import * as React from 'react';
import { DragEvent as ReactDragEvent, DragEventHandler as ReactDragEventHandler } from 'react';
import * as ReactDOM from 'react-dom';
import FileDrop from 'react-file-drop';

import '../dist/style.css';

var loadData = function (input, callback) {

  if (input instanceof File) {
    var reader = new FileReader();
    reader.onload = function () {
      return callback(reader.result); // no error
    }.bind(this);
    reader.readAsArrayBuffer(input);
  } else {
    var xhr = new XMLHttpRequest();
    xhr.open('GET', input, true);
    xhr.responseType = 'arraybuffer';
    xhr.onload = function (e) {
      if (xhr.status === 200) {
        return callback(xhr.response); // no error
      } else {

      }
    }.bind(this);
    xhr.onerror = function () {

    };
    xhr.onabort = function () {

    };
    xhr.send();
  }
}


class Demo extends React.Component {
  // handleFrameDragEnter = (event:DragEvent) => {
  //   console.log('handleFrameDragEnter', event);
  // }
  //
  // handleFrameDragLeave = (event:DragEvent) => {
  //   console.log('handleFrameDragLeave', event);
  // }
  //
  // handleFrameDrop = (event:DragEvent) => {
  //   console.log('handleFrameDrop', event);
  // }
  //
  // handleDragOver:ReactDragEventHandler<HTMLDivElement> = (event) => {
  //   console.log('handleDragOver', event);
  // }
  //
  // handleDragLeave:ReactDragEventHandler<HTMLDivElement> = (event) => {
  //   console.log('handleDragLeave', event);
  // }


  handleDrop = (files: FileList, event: ReactDragEvent<HTMLDivElement>) => {
    console.log('handleDrop!', files, event);

    loadData(files[0], function (data) {
      console.log(data);
      var dataView = new Uint8Array(data)
      window['FS'].writeFile("/in.wav", dataView);
      try {
        window['Module'].ccall('mainf', null, null);

      }
      catch (e) {
        console.log(e);  
      }


    })
  }

  render() {
    var styles = { border: '1px solid black', width: 600, color: 'black', padding: 20 };
    return (
      <div id="react-file-drop-demo" style={styles}>
        <FileDrop onDrop={this.handleDrop}>
          Drop some files here!
        </FileDrop>
      </div>
    );
  }
}

const dest = document.getElementById('app');
const content = <Demo />;


window['Module'].TOTAL_MEMORY = 16777216 * 4;

window['Module'].onMainLoopFinished = function () {
  console.log("Yes we're done!");
      var contents = window['FS'].readFile('/out.wav', { encoding: 'binary' });
      var blob = new Blob([contents], { type: "audio/wav" });
      var a = document.createElement("a");
      document.body.appendChild(a);
      //a.style = "display: none";
      var url = window.URL.createObjectURL(blob);
      a.href = url
      a.download = "output.wav"
      //a.click()
}


window['Module'].onRuntimeInitialized = function () {
  ReactDOM.render(content, dest);
}
