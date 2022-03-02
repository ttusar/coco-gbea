import pandas as pd
import os


DIM_MULTIPLIER = 1000
VALID_LINE_LENGTH = 10


def get_evaluation_num(f_tdat):
    """
    Retrieve the last number of evaluations from the tdat file. In case of any issues, return 0.
    """
    lines = f_tdat.readlines()[::-1]
    for line in lines:
        if len(line) > VALID_LINE_LENGTH:
            try:
                num_evaluations = int(line.split(' ')[0])
            except TypeError:
                num_evaluations = 0
            return num_evaluations
    return 0


def get_experiment_data(suite_name):
    """
    Retrieve information about experiments performed on a cluster that produces out files in
    addition to the usual coco result files.
    """
    file_in = f'{suite_name}-batches.txt'
    file_out = f'{suite_name}-results.txt'
    folder_output = f'{suite_name}-output'
    # Dictionaries to store information about the results where batch serves as key
    batch_out = {}
    batch_file_size = {}
    batch_evaluations = {}
    # Dictionaries to store information about the results where out name serves as key
    out_error = {}
    out_file_size = {}
    # Load information about the batches
    df = pd.read_csv(file_in, sep=' ')
    for file in os.listdir(folder_output):
        # Iterate over out files
        out_file_name = os.path.join(folder_output, file)
        with open(out_file_name, 'r', encoding='utf8') as f:
            out_name = os.path.splitext(os.path.basename(out_file_name))[0]
            # Store the size of the out file
            out_file_size.update({out_name: os.path.getsize(out_file_name)})
            # Find the batch number in the out file
            batch = -1
            for line in f:
                if f'Job, batch ID:' in line:
                    batch = int(line.split(' ')[-1])
                    batch_out.update({batch: out_name})
                    break
                elif len(line.split()) == 1 and '/' not in line:
                    batch = int(line)
                    batch_out.update({batch: out_name})
                    break
            # Find the size of the info file
            folder = os.path.join(f'{suite_name}-results', f'{suite_name}-batch-{batch}')
            try:
                for file_info in os.listdir(folder):
                    if file_info.endswith('.info'):
                        batch_file_size.update({
                            batch: os.path.getsize(os.path.join(folder, file_info))
                        })
            except FileNotFoundError:
                batch_file_size.update({batch: 'File missing'})
            # Find how many evaluations were performed from the tdat file
            function_num = df.loc[df['batch'] == batch, 'function'].values[0]
            dimension = df.loc[df['batch'] == batch, 'dimension'].values[0]
            folder = os.path.join(f'{suite_name}-results', f'{suite_name}-batch-{batch}',
                                  f'data_f{function_num}')
            batch_evaluations.update({batch: 0})
            try:
                for file_tdat in os.listdir(folder):
                    if file_tdat.endswith('.tdat'):
                        with open(os.path.join(folder, file_tdat)) as f_tdat:
                            num_evaluations = get_evaluation_num(f_tdat)
                        batch_evaluations.update({
                            batch: num_evaluations / (DIM_MULTIPLIER * dimension)
                        })
            except FileNotFoundError:
                pass
            # Find the error status in the out_name file
            file_contents = f.read()
            error_loc = file_contents.lower().find('error')
            if 'TIME LIMIT' in file_contents:
                out_error.update({out_name: 'Cancelled due to time limit'})
            elif error_loc > 0:
                eol_loc = file_contents[error_loc:].find('\n')
                error_loc_max = min(error_loc + 200, error_loc + eol_loc, len(file_contents))
                out_error.update({out_name: file_contents[error_loc: error_loc_max
                                            ].replace('"', '').replace('\'', '')})
            elif 'Stopped socket sever on port' in file_contents:
                out_error.update({out_name: 'Success'})

    df['info_size'] = -1
    df['eval_ratio'] = 0
    df['out_name'] = -1
    df['out_size'] = -1
    df['status'] = 'Success'
    for batch in df['batch'].tolist():
        df.loc[df['batch'] == batch, 'out_name'] = batch_out[batch]
        df.loc[df['batch'] == batch, 'info_size'] = batch_file_size[batch]
        df.loc[df['batch'] == batch, 'eval_ratio'] = f'{batch_evaluations[batch]:.4f}'
    for out_name in df['out_name'].tolist():
        df.loc[df['out_name'] == out_name, 'status'] = out_error[out_name]
        df.loc[df['out_name'] == out_name, 'out_size'] = out_file_size[out_name]
    df.to_csv(file_out, index=False)


if __name__ == '__main__':
    suite_name = 'rw-mario-gan'
    get_experiment_data(suite_name)
    suite_name = 'rw-top-trumps'
    get_experiment_data(suite_name)
